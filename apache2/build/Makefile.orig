#
# Makefile for mod_auth_cookie_(my)sql2
#

# "apxs" is searched in all directories of your PATH variable.
# If apxs cannot be found during the make process, you can adjust
# the path, i.e.:
#APXS = /usr/local/apache2/bin/apxs
APXS = apxs2

# mysql_config is searched in all directories of your PATH variable.
# If apxs cannot be found during the make process, you can adjust the
# the path, i.e.:
# MYSQCPPFLAGS = `/opt/mysql/bin/mysql_config --include`
# MYSQLDFLAGS  = `/opt/mysql/bin/mysql_config --libs`
MYSQCPPFLAGS = `mysql_config --include`
MYSQLDFLAGS  = `mysql_config --libs`

############### Please don't change anything below this line ###############

MODULE_NAME = auth_cookie_mysql2
APACHE_MODULE = mod_auth_cookie_mysql2.so

SRCS = mod_auth_cookie_sql2.c mod_auth_cookie_sql2_mysql.c
OBJS = mod_auth_cookie_sql2.o mod_auth_cookie_sql2_mysql.o

RM = rm -f
LN = ln -sf
CP = cp -f


#CFLAGS += -DNDEBUG
#CFLAGS +=  -DDEBUG

CFLAGS = -Wc,-Wall $(MYSQCPPFLAGS) -DMODULE_NAME=$(MODULE_NAME) -DMODULE_NAME_module=$(MODULE_NAME)_module
LDFLAGS = $(MYSQLDFLAGS) -o $(APACHE_MODULE)

default: all

all: $(APACHE_MODULE)

$(APACHE_MODULE): $(SRCS)
	$(APXS) -c $(CFLAGS) $(LDFLAGS) $(SRCS)

install: all
	$(APXS) -i -a -n $(MODULE_NAME) ./.libs/$(APACHE_MODULE)

clean:
	$(RM) $(OBJS) $(APACHE_MODULE)
