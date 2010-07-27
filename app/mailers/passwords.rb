PasswdApp.mailer :passwords do
  email :info do |send_to, email_to, pw_text|
    from    configatron.admin_email 
    to      send_to
    subject 'Kentfield School District login information'
    locals  :email_to => email_to, :pw_text => pw_text
    render  'passwords/info'
  end
end