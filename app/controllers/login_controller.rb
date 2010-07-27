require 'uuidtools'

PasswdApp.controllers :login do
  layout :login
  
  get :login, :map => '/login' do
    @login = ''
    @url = params[:url]
    render 'login/login.html'
  end
  
  post :login, :map => '/login' do
    @login = params[:login]
    if AuthCookieSession.authenticated?(@login, params[:password])
      sesskey  = SecureRandom.hex(16)
      remoteip = env['REMOTE_ADDR']
      expiry   = Time.now.to_i + 86400*180
      if AuthCookieSession.create_session(@login, sesskey, remoteip, expiry)
        response.set_cookie(configatron.cookie_name, 
          :value   => sesskey,
          :domain  => configatron.cookie_domain, 
          :path    => configatron.cookie_path, 
          :expires => Time.at(expiry) )
        redirect (@url || configatron.default_url)
        return
      else
        @errors = 'Internal error'
      end
    else
      @errors = 'Incorrect username or password'
    end
    render 'login/login.html'
  end
  
  get :logout, :map => '/logout' do
    cookie_name = configatron.cookie_name
    sesskey = request.cookies[cookie_name]
    if AuthCookieSession.destroy_session(sesskey)
      response.set_cookie(cookie_name, 
        :value   => '',
        :domain  => configatron.cookie_domain, 
        :path    => configatron.cookie_path, 
        :expires => Time.at(Time.now.to_i - 86400))
    end
    @logout_link = configatron.logout_link
    render 'login/logout.html'
  end
end
