# SHOW CREATE TABLE auth_cookie_sessions
CREATE TABLE `auth_cookie_sessions` (
    `sessname` varchar(32) NOT NULL,
    `sesskey` varchar(32) NOT NULL,
    `remoteip` varchar(15) NOT NULL,
    `expiry` int(11) DEFAULT '0',
    `username` varchar(60) NOT NULL,
    PRIMARY KEY (`sessname`,`sesskey`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
