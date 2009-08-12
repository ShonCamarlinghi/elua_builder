-- User application controller
--------------------------------------------------------------------
-- Using: Just define bellow lua functions required by the action
-- and if any was required, the 'index' function will be used.
--------------------------------------------------------------------

function index()
	render("index.lp")
end

function authenticate()
	user_aut = cgilua.POST
	local User = require "user.model"
	local val = require "validation"
	local validator = val.implement.new(user_aut)
	
	validator:validate('login',locale_register.validator.login, val.checks.isNotEmpty)
	validator:validate('passwd', locale_register.validator.passwd, val.checks.isNotEmpty)
	validator:validate('login',locale_register.validator.authenticate, function()
																	if val.checks.isNotEmpty(user_aut.login) and val.checks.isNotEmpty(user_aut.passwd)  then 
																		return User.checkUser(user_aut.login,user_aut.passwd)
																	else
																		return true
																	end
																end)
	
	if(validator:isValid())then
		User.authenticate(user_aut)
		redirect({control="builder", act="index"})
	else
		flash.set('validationMessages',validator:htmlMessages())
		index()
	end
end

function logout()
	cgilua.cookies.delete ('system_cookie')
	redirect({control="user", act="index"})
end

function create()
	User = require "user.model"
	user = user_obj or {}
	render("create.lp")
end

function register()
	user_obj = cgilua.POST
	
	local val = require "validation"
	local UserModel = require "user.model"
	local validator = val.implement.new(user_obj)
			
	validator:validate('name',locale_register.validator.name,val.checks.isNotEmpty)
	
	validator:validate('email',locale_register.validator.email_none, val.checks.isNotEmpty)
	validator:validate('email',locale_register.validator.valid_email, val.checks.isEmail)
	validator:validate('email',locale_register.validator.confirm_email, val.checks.isEqual,user_obj.co_email)
	
	validator:validate('login',locale_register.validator.login, val.checks.isNotEmpty)
	validator:validate('login',locale_register.validator.login_min, val.checks.minLength,5)
	
	validator:validate('passwd',locale_register.validator.passwd, val.checks.isNotEmpty)
	validator:validate('passwd',locale_register.validator.passwd_min, val.checks.sizeString,8)
	validator:validate('passwd',locale_register.validator.confirm_passwd, val.checks.isEqual,user_obj.co_passwd)
	validator:validate('login',locale_register.validator.checkNotExistLogin, UserModel.checkNotExistLogin)
		
	if(validator:isValid())then
		UserModel.save(user_obj)
		flash.set('notice',locale_register.validator.notice)
		redirect({control="user", act="index"})
	else
		flash.set('validationMessages',validator:htmlMessages())
		create()
	end
end