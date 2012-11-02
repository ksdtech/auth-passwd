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
 *
 *  Version history:
 *  Version 0.2 - 28.07.2004  - first release - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 0.3 - 03.08.2004  - code fixup - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 0.4 - 05.04.2005  - bug fixes and redirect modified - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 0.5 - 01.06.2005  - mysql_close() fix - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 0.6 - 20.12.2005  - mysql_otions() added, mysql query modification - Thimo Eichstaedt <apache.mod@digithi.de> 
 *  Version 0.7 - 04.01.2006  - load return codes with default values - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 0.8 - 26.03.2007  - code cleanup, small memory leak fixed - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 0.9 - 23.04.2007  - code/documentation cleanup, persistent database connections added, rename of config vars - Thimo Eichstaedt <apache-mod@digithi.de>
 *  Version 1.0 - 18.06.2009  - AuthCookieSQL_AdditionalSQL added, minor code cleanup - Thimo Eichstaedt <apache-mod@digithi.de> 
 *
 */

/********************************************************************************
 *                                includes                                      *
 ********************************************************************************/

#include "apr_strings.h"
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_request.h"
#include "http_log.h"
#include "mod_auth_cookie_sql2.h"

#include <time.h>               /* for time */
#include <string.h>             /* for strncmp */

module AP_MODULE_DECLARE_DATA MODULE_NAME_module;

/********************************************************************************
 *                      public function declarations                            *
 ********************************************************************************/
static int auth_cookie_sql2_authenticate_user (request_rec *);
static void *auth_cookie_sql2_create_auth_dir_config(apr_pool_t *, char *);

/********************************************************************************
 *                      local function declarations                             *
 ********************************************************************************/
static int check_valid_cookie(request_rec *, auth_cookie_sql2_config_rec *);
static int do_redirect(request_rec *);

/********************************************************************************
 *                            local variables                                   *
 ********************************************************************************/
static auth_cookie_sql2_config_rec default_config_rec = {
    0,		/* are we activated ? */
    NULL,	/* Cookie Name */
    NULL,	/* Database host */
    NULL,	/* Database user */
    NULL,	/* Database password */
    NULL,	/* Database name */
    NULL,	/* Database table */
    0,		/* Persistent connection */
    NULL,	/* Database username field */
    NULL,	/* Database sessname field */
    NULL,	/* Database sessval field */
    NULL,	/* Database expiry information field */
    NULL,	/* Database remote ip information field */
    NULL,	/* SQL addon */
    NULL,	/* goto URL if failure */
};    

/********************************************************************************
 *                               functions                                      *
 ********************************************************************************/

static void *auth_cookie_sql2_create_auth_dir_config(apr_pool_t *p, char *d) {
    auth_cookie_sql2_config_rec *conf = apr_pcalloc(p, sizeof(*conf));
    
    if (conf) {
	// Set default values
	*conf=default_config_rec;
    }
    return conf;
}

/* this function is called when the child or module is cleared */
static apr_status_t auth_cookie_sql2_child_exit(void *data) {
#ifdef AUTH_COOKIE_SQL2_DEBUG
    server_rec *s = (server_rec *) data;
#endif

#ifdef AUTH_COOKIE_SQL2_DEBUG
    ap_log_error(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, s, ERRTAG "child exit called.");
#endif

    close_db(NULL, NULL, 1);
    return APR_SUCCESS;
}

/* this function is called the the module is initialized by apache */
static void auth_cookie_sql2_child_init(apr_pool_t *p, server_rec *s) {

#ifdef AUTH_COOKIE_SQL2_DEBUG
    ap_log_error(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, s, ERRTAG "child init called.");
#endif

    apr_pool_cleanup_register(p,
			      s,
			      auth_cookie_sql2_child_exit,
			      auth_cookie_sql2_child_exit
			      );
}

/* send a redirect to the browser */
static int do_redirect(request_rec *r) {
    auth_cookie_sql2_config_rec *conf = ap_get_module_config(r->per_dir_config, &MODULE_NAME_module);
    char *redirect = apr_psprintf(r->pool, "%s?r=%s", conf->failureurl, r->uri);
											    
    if (redirect) {
	apr_table_setn(r->headers_out, "Location", redirect);
	return HTTP_MOVED_TEMPORARILY;
    } else {
	return HTTP_INTERNAL_SERVER_ERROR;
    }
}

/* check for a valid cookie */
static int check_valid_cookie(request_rec *r, auth_cookie_sql2_config_rec *conf) {
    const char *cookieptr;
    char *cookies, *value;
    char username[MAX_USERNAME_LEN + 1];
    time_t tc;
    int db_ret=RET_UNAUTHORIZED; //default: unauthorized

    // grab pointer to cookie informations
    if ((cookieptr = apr_table_get(r->headers_in, "Cookie")) == NULL) {

	// no cookies found at all, so return with failure/unauthorized
	if (conf->failureurl) {
		return do_redirect(r);
	} else {
		return HTTP_UNAUTHORIZED;
	}
    }

    // make a copy of cookies, so we can do what we want (strtok, ...)
    if ((cookies = apr_palloc(r->pool, strlen(cookieptr) + 2)) == NULL) {
	// error
	return HTTP_INTERNAL_SERVER_ERROR;
    }
    strcpy (cookies, cookieptr);
    cookies[0 + strlen(cookieptr)] = ';';
    cookies[1 + strlen(cookieptr)] = '\0';

    // fetch current time;
    tc = time(NULL);

	// now check for cookies
    if (conf->cookiename) {
	// a cookiename is given, we are searching only for THIS cookie in the DB

	// find cookie we are searching for
	if ((cookies = strstr(cookies, conf->cookiename)) != NULL) {
	    // found cookie

	    if ((value = strchr (cookies, '=')) != NULL) {
		// found beginning of cookieval
		if ((value = strtok(value+1, " ;\n\r\t\f")) != NULL) {

#ifdef AUTH_COOKIE_SQL2_DEBUG
		    ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "valid cookie found, data: %s", cookies);
#endif

		    db_ret=check_against_db(conf, r, conf->cookiename, value, username,r->connection->remote_ip, conf->sql_addon, tc);
		}
	    }
	}
    } else {
	// no cookiename is given, lets try all cookies the client gave us
	for (cookies = strtok(cookies, " ;\n\r\t\f"); (cookies); cookies = strtok(NULL, " ;\n\r\t\f")) {

	    /* content of cookies should now be like "aaa=bbb"
	       check this and, if not, continue search for a valid cookie */
	    if ((value = strchr (cookies, '=')) == NULL) {
		continue;
	    }

#ifdef AUTH_COOKIE_SQL2_DEBUG
	    ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "testing against valid cookie: %s", cookies);
#endif

	    /* modify aaa=bbb\0 to aaa\0bbb\0, so cookie contains aaa\0
	       and value contains bbb\0 */
	    *value = '\0';
	    value++;

	    if ((db_ret=check_against_db(conf, r, cookies, value, username, r->connection->remote_ip, conf->sql_addon, tc)) == RET_AUTHORIZED) {
		// found valid cookie

#ifdef AUTH_COOKIE_SQL2_DEBUG
		ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "valid cookie found");
#endif

		break;
	    }
	}
    }

    // handle return codes
    switch (db_ret) {
        case RET_AUTHORIZED:
    	    // valid cookie found
	    r->user = apr_pstrdup(r->pool,username);
	    r->ap_auth_type="Cookie";
	    return OK;
	    break;
	case RET_UNAUTHORIZED:
	    // no valid information in database found
	    if (conf->failureurl) {
		return do_redirect(r);
	    } else {
		return HTTP_UNAUTHORIZED;
	    }
	    break;
	default:
	    return DECLINED;
	    break;
    }
}

/* try to authenticate the user */
static int auth_cookie_sql2_authenticate_user (request_rec *r) {
    auth_cookie_sql2_config_rec *conf = ap_get_module_config(r->per_dir_config, &MODULE_NAME_module);
    
    if (! conf->activated) {
	// not active
	return DECLINED;
    }

    if ( !conf->dbhost || !conf->dbuser || !conf->dbpassword || !conf->dbname || !conf->dbtable ) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "please check database connect information, some are missing");
	return DECLINED;
    }
    
    if ( !conf->dbusername_field || !conf->dbsessname_field || !conf->dbsessval_field) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "please check database field names, some are missing");
	return DECLINED;
    }

#ifdef AUTH_COOKIE_SQL2_DEBUG
    ap_log_rerror(APLOG_MARK, APLOG_ERR|APLOG_NOERRNO, 0, r, ERRTAG "module started.");
#endif
    
    return check_valid_cookie(r,conf);
    
}    


/* Module data */
static const command_rec auth_commands[] = {

  AP_INIT_FLAG("AuthCookieSql", ap_set_flag_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, activated),
    WHERE_ALLOWED, "Set to on to activate Cookie Auth"),


  AP_INIT_TAKE1("AuthCookieSql_CookieName", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, cookiename),
    WHERE_ALLOWED, "Name of the cookie"),

  AP_INIT_TAKE1("AuthCookieSql_DBHost", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbhost),
    WHERE_ALLOWED, "Host on which DB server resides"),

  AP_INIT_TAKE1("AuthCookieSql_DBUser", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbuser),
    WHERE_ALLOWED, "Username for DB access"),

  AP_INIT_TAKE1("AuthCookieSql_DBPassword", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbpassword),
    WHERE_ALLOWED, "Password for DB access"),
    
  AP_INIT_TAKE1("AuthCookieSql_DBName", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbname),
    WHERE_ALLOWED, "Database name"),

  AP_INIT_TAKE1("AuthCookieSql_DBTable", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbtable),
    WHERE_ALLOWED, "Table name in database"),

  AP_INIT_FLAG("AuthCookieSql_DBPersistent", ap_set_flag_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbpersistent),
    WHERE_ALLOWED, "If enabled connection to database is held open during requests"),

  AP_INIT_TAKE1("AuthCookieSql_UsernameField", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbusername_field),
    WHERE_ALLOWED, "Field name in database where username is hold"),

  AP_INIT_TAKE1("AuthCookieSql_SessnameField", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbsessname_field),
    WHERE_ALLOWED, "Field name in database where session name is hold"),

  AP_INIT_TAKE1("AuthCookieSql_SessvalField", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbsessval_field),
    WHERE_ALLOWED, "Field name in database where session key is hold"),

  AP_INIT_TAKE1("AuthCookieSql_ExpiryField", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbexpiry_field),
    WHERE_ALLOWED, "Field name in database where expiry information is hold"),

  AP_INIT_TAKE1("AuthCookieSql_RemoteIPField", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, dbremoteip_field),
    WHERE_ALLOWED, "Field name in database where remote ip information is hold"),

  AP_INIT_TAKE1("AuthCookieSql_AdditionalSql", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, sql_addon),
    WHERE_ALLOWED, "SQL clause which is appended after the SQL statement"),

  AP_INIT_TAKE1("AuthCookieSql_FailureURL", ap_set_string_slot,
    (void *)APR_OFFSETOF(auth_cookie_sql2_config_rec, failureurl),
    WHERE_ALLOWED, "URL where to go to when auth not successful"),
    
  { NULL }
};    

/* register hooks at the apache server */
static void auth_cookie_sql2_register_hooks(apr_pool_t *p) {

    /* Hook in and great, we are the first :) */
    ap_hook_check_user_id(auth_cookie_sql2_authenticate_user,NULL,NULL,APR_HOOK_FIRST);

    /* hook in for initialisation */
    ap_hook_child_init(auth_cookie_sql2_child_init, NULL, NULL, APR_HOOK_MIDDLE);
}

/* module data */
module AP_MODULE_DECLARE_DATA MODULE_NAME_module = {
    STANDARD20_MODULE_STUFF,
    auth_cookie_sql2_create_auth_dir_config,	/* per-directory config creater */
    NULL,                       		/* dir merger --- default is to override */
    NULL,                       		/* server config creator */
    NULL,                       		/* server config merger */
    auth_commands,              		/* command table */
    auth_cookie_sql2_register_hooks              /* set up other request processing hooks */
};
