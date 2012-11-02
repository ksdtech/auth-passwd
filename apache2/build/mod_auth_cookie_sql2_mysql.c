/*  Copyright 2004-2007 Thimo Eichstaedt <apache-mod@digithi.de>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Based on code from
 *      Dirk.vanGulik@jrc.it  - original module
 *      mjd@pobox.com         - bug fixes, conversion from mSQL to MySQL
 *
 */
 
/********************************************************************************
 *                                includes                                      *
 ********************************************************************************/
#include <string.h> 
#include <mysql.h>
#include "apr_strings.h"
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_request.h"
#include "http_log.h"

#include "mod_auth_cookie_sql2.h"

static MYSQL *dbh = NULL;

/********************************************************************************
 *                               functions                                      *
 ********************************************************************************/

/* try to open connection */
int open_db(auth_cookie_sql2_config_rec *conf, request_rec *r) {

    if (dbh != NULL) {
	if (mysql_ping(dbh) == 0) {
	    return RET_OK;
	} else {
	    ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "database connection died, trying to establish a new one.");
	    mysql_close(dbh);
	    dbh = NULL;
	}
    }

    if ((dbh=mysql_init(NULL)) == NULL) {
	return RET_ERR;
    }

    mysql_options(dbh,MYSQL_READ_DEFAULT_GROUP,MY_MYSQL_APPNAME);
    
    if (mysql_real_connect(dbh, conf->dbhost, conf->dbuser, conf->dbpassword, conf->dbname, 0, NULL, 0) == NULL) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "couldn't connect to database: %s", mysql_error(dbh));

	return RET_ERR;
    } else {
	return RET_OK;
    }
}

/* close the database connection */
int close_db(auth_cookie_sql2_config_rec *conf, request_rec *r, int force) {
    if (dbh == NULL) {
	// do nothing
    } else if (conf == NULL) {
	mysql_close(dbh);
	dbh=NULL;
    } else if (conf->dbpersistent == 0 || force == 1) {
	mysql_close(dbh);
	dbh=NULL;
    }

    return RET_OK;
}

/* check given cookie against db */
int check_against_db(auth_cookie_sql2_config_rec *conf, request_rec *r, char *cookiename, char *cookieval, char *username, char *remoteip, char *addon, time_t tc) {
    MYSQL_RES *res;
    MYSQL_ROW row;
    apr_pool_t *p = r->pool;
    char *esc_cookiename, *esc_cookieval;
    char *query, *queryopt;
    int ulen;
    int ret=RET_ERR; // default

    if (open_db(conf,r) != RET_OK) {
	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }

    /* escape cookiename, size of cookiename can grow double as big as before */
    ulen=strlen(cookiename);
    if (! (esc_cookiename = apr_palloc(p, (ulen*2) + 1))) {
	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }

    mysql_real_escape_string(dbh, esc_cookiename, cookiename, ulen);
    
    /* escape cookieval, can grow to double size, too */
    ulen=strlen(cookieval);
    if ((esc_cookieval = apr_palloc(p, (ulen*2) + 1)) == NULL) {
	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }
    
    mysql_real_escape_string(dbh, esc_cookieval, cookieval, ulen);    
    
    /* prepare query string, queryopt contains optional arguments */
    if ((queryopt = apr_palloc(r->pool,sizeof(char))) == NULL) {
	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }

    *queryopt = '\0';
    
    /* Check for cookie Expiry ? */
    if (conf->dbexpiry_field) {
	queryopt = apr_psprintf(p, "%s AND %s > %lu", queryopt, conf->dbexpiry_field,tc);
    }
    
    /* Check for right remote ip ? */
    if (conf->dbremoteip_field) {
	queryopt = apr_psprintf(p, "%s AND %s='%s'", queryopt, conf->dbremoteip_field, remoteip);
    }
    
    if (addon) {
	queryopt = apr_psprintf(p,"%s %s", queryopt, addon);
    }
    
    /* Generate query */
    query = apr_psprintf(p, "SELECT %s FROM %s WHERE %s='%s' AND %s='%s'%s",
	    conf->dbusername_field,
	    conf->dbtable,
	    conf->dbsessname_field,
	    esc_cookiename,
	    conf->dbsessval_field,
	    esc_cookieval,
	    queryopt);

    if (query == NULL) {
	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }

    if (mysql_query(dbh, query) != 0) {  
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "error in MySQL query \"%s\": %s", query, mysql_error(dbh));

	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }

    if ((res = mysql_store_result(dbh)) == NULL) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "couldn't store query result: %s", mysql_error(dbh));

	ret=RET_ERR;
	goto check_against_db_mysql_close;
    }

    /* if any other number of results than 1 found, clean up and return with 0 */
    if (mysql_num_rows(res) != 1) {

	ret=RET_UNAUTHORIZED;
	goto check_against_db_mysql_free_result;
    }
	
    /* got a result, fetch row */
    if ((row = mysql_fetch_row(res)) == NULL) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "couldn't fetch row: %s", mysql_error(dbh));

	ret=RET_ERR;
	goto check_against_db_mysql_free_result;
    }
    
    /* check for length of row */
    if (strlen (row[0]) > MAX_USERNAME_LEN) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "fetched username from DB, but is longer than max length %d", MAX_USERNAME_LEN);

	ret=RET_ERR;
	goto check_against_db_mysql_free_result;
    }
	
    /* all tests passed, copy content of row[0] to username */
    strcpy(username, row[0]);
    ret=RET_AUTHORIZED;

check_against_db_mysql_free_result:
    mysql_free_result(res);

check_against_db_mysql_close:
    close_db(conf,r,0);
    return ret;
}
