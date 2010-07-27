PasswdApp.helpers do
  def password_info_html(pwinfo)
    html = [ ]
    pwinfo.each do |pwline|
      html << "Login information for these site(s): #{pwline[:desc]}"
      html << "Web address(es): #{pwline[:urls].join(' or ')}"
      html << "Name: #{pwline[:detail]}"
      html << "User name: #{pwline[:username]}"
      html << "Email address: #{pwline[:email]}" if pwline[:email]
      html << "Password: #{pwline[:password]}"
      html << ''
    end
    '<p>' + html.join('<br />') + '</p>'
  end

  def password_info_text(pwinfo)
    txt = [ ]
    pwinfo.each do |pwline|
      txt << "  Login information for these site(s): #{pwline[:desc]}"
      txt << "  Web address(es): #{pwline[:urls].join(' or ')}"
      txt << "  Name: #{pwline[:detail]}"
      txt << "  User name: #{pwline[:username]}"
      txt << "  Email address: #{pwline[:email]}" if pwline[:email]
      txt << "  Password: #{pwline[:password]}"
      txt << ''
    end
    txt.join("\n")
  end
end