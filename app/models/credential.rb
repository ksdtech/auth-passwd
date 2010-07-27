class Credential
  include DataMapper::Resource
  include ::Encryption
  belongs_to :contact

  property :id, Serial
  property :username, String
  property :encrypted_password, String
  property :salt, String
  
  attr_accessor :password
  
  before :save, :encrypt_password
  
  def cleartext_password
    raise "no password" unless self.encrypted_password && self.salt
    Credential.decrypt_password(self.encrypted_password, self.salt)
  end
  
  def encrypt_password
    if (self.salt.nil? || self.encrypted_password.nil?) && !self.password.nil?
      self.salt = Credential.generate_salt
      self.encrypted_password = Credential.encrypt_password(self.password.downcase, self.salt)
      self.password = nil
    end
  end

  def authenticated?(password)
    !password.nil? && !self.salt.nil? && !self.encrypted_password.nil? && 
      self.encrptyed_password == Credential.encrypt_password(password.downcase, self.salt)
  end
  
  def self.authenticated?(username, password)
    cred = Credential.first(:username => username)
    cred && cred.authenticated?(password)
  end
end
