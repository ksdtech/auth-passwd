class AuthCookieSession
  include DataMapper::Resource
  property :sessname, String,  :length => 32, :key => true
  property :sesskey,  String,  :length => 32, :key => true
  property :remoteip, String,  :length => 15, :required => true
  property :expiry,   Integer, :default => 0
  property :username, String,  :length => 60, :required => true
  
  class << self
    def authenticated?(login, password)
      Credential.authenticated?(login, password)
    end

    def purge_stale_sessions
      AuthCookieSession.all(:expiry.lt => Time.now.to_i).destroy
    end
    
    def create_session(username, sesskey, remoteip, expiry)
      purge_stale_sessions
      acs = AuthCookieSession.create(
        :username => username,
        :sessname => configatron.cookie_name,
        :sesskey  => sesskey,
        :remoteip => remoteip,
        :expiry   => expiry)
    end

    def destroy_session(sesskey)
      acs = AuthCookieSession.get(configatron.cookie_name, sesskey)
      rc = acs && acs.destroy
      purge_stale_sessions
      rc
    end
  end
end
