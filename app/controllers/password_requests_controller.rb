PasswdApp.controllers :password_requests do
  layout :password_requests
  
  get :index do
    render 'password_requests/index.html'
  end
  
  get :new do
    render "password_requests/new.html"
  end
  
  post :new do
    view_to_render = 'new.html'
    email_addr = params[:email]
    if email_addr.nil? || email_addr.empty?
      @missing = true
    else
      m = email_addr.match(/^webmaster\+(.+)$/)
      if m
        email_addr = $1
        send_to = 'webmaster@kentfieldschools.org'
      else
        send_to = email_addr
      end
      email_addr.strip!
      pwinfo = EmailReference.get_password_info(email_addr)
      unless pwinfo
        @failed = email_addr
      else
        @email_to = email_addr
        begin
          pw_text = password_info_text(pwinfo)
          deliver(:passwords, :info, send_to, @email_to, pw_text)
          @succeeded = true
        rescue
          raise
          # SMTP error
          @internal_error = $!
        end
      end
      view_to_render = 'index.html'
    end
    render "password_requests/#{view_to_render}"
  end
end
