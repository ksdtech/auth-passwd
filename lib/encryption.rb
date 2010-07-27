require 'ezcrypto'
require 'configatron'

module Encryption
  SALT_CHARS = ['.', '/', '0'..'9', 'A'..'Z', 'a'..'z'].collect do |x|
    x.to_a
  end.flatten
  
  module ClassMethods
    def generate_salt(length=8)
      salt = ""
      length.times {salt << SALT_CHARS[rand(SALT_CHARS.length)]}
      salt
    end

    def encrypt_password(cleartext_password, salt)
      key = EzCrypto::Key.with_password(configatron.encryption_key, salt)
      key.encrypt64(cleartext_password)
    end

    def decrypt_password(encrypted_password, salt)
      key = EzCrypto::Key.with_password(configatron.encryption_key, salt)
      key.decrypt64(encrypted_password)
    end
  end
  
  def self.included(base)
    base.extend(ClassMethods)
  end
end
