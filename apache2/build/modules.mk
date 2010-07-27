mod_auth_cookie_mysql.la: mod_auth_cookie_mysql.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_auth_cookie_mysql.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_auth_cookie_mysql.la
