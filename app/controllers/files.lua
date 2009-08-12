-- Files application controller
--------------------------------------------------------------------
-- Using: Just define bellow lua functions required by the action
-- and if any was required, the 'index' function will be used.
--------------------------------------------------------------------

function repository()
	local startIndex = cgilua.QUERY.startIndex
	local results = cgilua.QUERY.results
	local sort = cgilua.QUERY.sort
	local dir  = cgilua.QUERY.dir
	local query = cgilua.QUERY
	local build_id = cgilua.QUERY.build_id
	local FileModel = require("file.model")
	
	local items = FileModel.getFiles()
	local build_files_index = FileModel.getFilesByBuildIndex(build_id)
	for _,v in pairs(items)do
		if type(build_files_index) == "table" then
			v.id = {id = v.id, checked = build_files_index[v.id]}
		else
			v.id = {id = v.id, checked = false}
		end
	end
		 
	local DT = require("dataTable")
	local rep = DT.repository:new(items,{startIndex=startIndex,results=results,sort=sort,dir=dir})
	 
	rep:response('text','plain','UTF-8')
end

function upload()
	local FileModel = require("file.model") 
	local file_upload = cgilua.POST.file
    if file_upload and next(file_upload) then
		local _, name = cgilua.splitonlast(file_upload.filename)
		local file = file_upload.file
        local dir = FileModel.checkDir()
		destination = io.open(dir.."/"..name, "wb")
		FileModel.save(name)
        if destination then
            local bytes = file:read("*a")
            destination:write(bytes)
            destination:close()
        end
	end
	redirect({control="builder", act="index"})
end

function delete()
	local UserModel = require "user.model"
	local id = cgilua.QUERY.id
	local user = UserModel.getCurrentUser()
	local file = db:selectall("*","files","id = "..id)
	filename = file[1].filename
	local path = CONFIG.MVC_UPLOAD..user.login.."/"..filename
	os.remove(path)
	local files = db:delete ("files","id = "..id)
	local files_build = db:delete ("builds_files", "file_id="..id)
	redirect({control="builder", act="index"})
end