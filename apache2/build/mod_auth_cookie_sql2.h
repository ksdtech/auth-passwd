/*  Copyright 2004-2007 Thimo Eichstaedt
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
 
#ifndef _MOD_AUTH_COOKIE_SQL2_H
#define _MOD_AUTH_COOKIE_SQL2_H

#define MAX_USERNAME_LEN 128

#define WHERE_ALLOWED OR_AUTHCFG

#define ERRTAG "Mod_Auth_Cookie_Mysql2 "
#define MY_MYSQL_APPNAME "ModAuthCookieMysql2"

#define RET_OK 1
#define RET_ERR -1

#define RET_UNAUTHORIZED 2
#define RET_AUTHORIZED 3

//#define AUTH_COOKIE_SQL2_DEBUG 1

typedef struct s_auth_cookie_sql2_config_struct {
    int  activated;
    char *cookiename;
    char *dbhost;
    char *dbuser;
    char *dbpassword;
    char *dbname;
    char *dbtable;
    int  dbpersistent;
    char *dbusername_field;
    char *dbsessname_field;
    char *dbsessval_field;
    char *dbexpiry_field;
    char *dbremoteip_field;
    char *sql_addon;    
    char *failureurl;
} auth_cookie_sql2_config_rec;


/* Private functions */
extern int check_against_db(auth_cookie_sql2_config_rec *, request_rec *, char *, char *, char *, char *, char *, time_t);
extern int open_db(auth_cookie_sql2_config_rec *, request_rec *);
extern int close_db(auth_cookie_sql2_config_rec *, request_rec *, int);

#endif // _MOD_AUTH_COOKIE_SQL2_H
