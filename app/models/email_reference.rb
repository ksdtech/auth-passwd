#= EmailReference
class EmailReference
  include DataMapper::Resource
  has n, :contacts

  property :email, String, :key => true
  
  def get_password_info
    info = [ ]
    family_emails = [ self.email ]
    parent_ids = [ ]
    staff_ids  = [ ]
    contacts.all(:type => 'Family').each do |fam|
      fam.student_references.each do |fs|
        next if fs.parent_of_student.nil?
        parent_ids << fs.parent_of_student.student_id
        family_emails << fs.parent_of_student.email_reference.email
      end
      fam.staff_references.each do |fs|
        next if fs.employee.nil?
        staff_ids << fs.employee.staff_id
        family_emails << fs.employee.email_reference.email
      end
    end
    family_emails = family_emails.uniq
    parent_ids = parent_ids.uniq
    staff_ids  = staff_ids.uniq
    puts "family_emails: #{family_emails.inspect}"
    puts "parent_ids: #{parent_ids.inspect}"
    puts "staff_ids: #{staff_ids.inspect}"
    
    unless staff_ids.empty?
      Employee.all(:conditions => ['staff_id IN ?', staff_ids]).each do |emp|
        info << { :desc => "Staff PowerSchool and file server login (and teacher websites)", 
          :urls => [ 'http://ps.kentfieldschools.org/teachers', 'http://kentweb.kentfieldschools.org' ],
          :detail => emp.full_name,
          :username => emp.credential.username, 
          :password => emp.credential.cleartext_password }
      end
    end
    
    unless parent_ids.empty?
      duplicates = { }
      ParentOfStudent.all(:conditions => ['student_id IN ?', parent_ids]).each do |ps|
        next if duplicates.key?(ps.student_id)
        duplicates[ps.student_id] = 1
        info << { :desc => "PowerSchool Parent Portal login (and teacher websites)",
          :urls => [ 'http://ps.kentfieldschools.org', 'http://kentweb.kentfieldschools.org' ],
          :detail => ps.full_name,
          :username => ps.credential.username,
          :password => ps.credential.cleartext_password }
      end
    end
    
    unless family_emails.empty?
      SchoolwiresUser.all(:conditions => ['email_reference_email in ?', family_emails]).each do |sw|
        info << { :desc => "Schoolwires login (and teacher websites)",
          :urls => [ 'http://www.kentfieldschools.org', 'http://kentweb.kentfieldschools.org' ],
          :detail => sw.full_name,
          :username => sw.schoolwires_username, 
          :email => sw.email_reference_email,
          :password => '(use Forgotten Sign-In Information? button to retrieve)' }
      end
    end
    
    info.empty? ? nil : info
  end
  
  def self.get_password_info(email)
    return nil unless email
    eref = EmailReference.get(email)
    eref ? eref.get_password_info : nil
  end
end
            