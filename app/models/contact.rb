#= Contact
class Contact
  include DataMapper::Resource
  property :id, Serial
  property :first_name, String
  property :last_name, String
  property :type, Discriminator

  has 1, :credential
  belongs_to :email_reference
  
  def full_name
    "#{first_name} #{last_name}".strip
  end
end

class BuiltIn < Contact
  property :built_in_id, Integer

  def to_s
    "BuiltIn #{first_name} #{last_name} [#{built_in_id}]"
  end
end

class SchoolwiresUser < Contact
  property :schoolwires_username, String
end

class Employee < Contact
  property :staff_id, Integer
  property :school_id, Integer

  has n, :staff_references, :model => 'FamilyStaff', :parent_key => [:staff_id]
  
  def to_s
    "Employee #{first_name} #{last_name} [#{staff_id}]"
  end
end

class Family < Contact
  property :family_id, Integer

  has n, :staff_references, :model => 'FamilyStaff', :parent_key => [:family_id]
  has n, :student_references, :model => 'FamilyStudent', :parent_key => [:family_id]

  def to_s
    "#{first_name} #{last_name} [#{family_id}]"
  end
end

class Student < Contact
  property :student_id, Integer
  property :school_id, Integer
  property :grade_level, Integer

  def to_s
    "Student #{first_name} #{last_name} (#{grade_level}) [#{student_id}]"
  end
end

class ParentOfStudent < Student
  has n, :family_references, :model => 'FamilyStudent', :parent_key => [:student_id]

  def to_s
    "Parent of #{first_name} #{last_name} (#{grade_level}) [#{student_id}]"
  end
end

class FamilyStaff
  include DataMapper::Resource
  property :id, Serial
  
  belongs_to :family, :parent_key => [:family_id]
  belongs_to :employee, :parent_key => [:staff_id]
end

class FamilyStudent
  include DataMapper::Resource
  property :id, Serial
  
  belongs_to :family, :parent_key => [:family_id]
  belongs_to :parent_of_student, :parent_key => [:student_id]
end
