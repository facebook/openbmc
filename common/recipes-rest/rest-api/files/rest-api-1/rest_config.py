import ConfigParser

configpath = '/etc/rest.cfg'
RestConfig = ConfigParser.ConfigParser()
RestConfig.read(configpath)
