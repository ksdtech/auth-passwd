##
##  Makefile -- Build procedure for sample auth_cookie_mysql Apache module
##  Autogenerated via ``apxs -n auth_cookie_mysql -g''.
##

builddir=.
#   PFZ: these were not generated correctly from apxs
top_srcdir=/usr/share/httpd
top_builddir=/usr/share/httpd
include /usr/share/httpd/build/special.mk

#   the used tools
APXS=apxs
APACHECTL=apachectl

#   PFZ: get mysql client files
MYSQL_INCLUDES=`mysql_config --include`
MYSQL_LIBS=`mysql_config --libs_r`

#   additional defines, includes and libraries
#DEFS=-Dmy_define=my_value
INCLUDES=$(MYSQL_INCLUDES)
LIBS=$(MYSQL_LIBS)
LT_LDFLAGS=$(MYSQL_LIBS)

#   the default target
all: local-shared-build

#   install the shared object file into Apache 
install: install-modules-yes

#   cleanup
clean:
	-rm -f mod_auth_cookie_mysql.o mod_auth_cookie_mysql.lo mod_auth_cookie_mysql.slo mod_auth_cookie_mysql.la 

#   simple test
test: reload
	lynx -mime_header http://localhost/auth_cookie_mysql

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop

