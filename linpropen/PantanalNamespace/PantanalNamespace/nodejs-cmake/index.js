const binding = require('./build/Release/binding.node')
function sucess(strSucess)
{
    console.log(strSucess);
}
function fail(code,data)
{
    console.log("code = "+ code+"data="+data );
}
function complete()
{
    console.log("complete");
}

binding.copy("copy","copy",sucess,fail,complete);
binding.move("copy","copy",sucess,fail,complete);
binding.list("copy",sucess,fail,complete);
binding.get("copy",false,sucess,fail,complete);
binding.delete("delete",sucess,fail,complete);
binding.writeText("delete","write","decode",false,sucess,fail,complete);
binding.writeArrayBuffer("delete","write",3,false,sucess,fail,complete);
binding.readText("read","copy",sucess,fail,complete);
binding.readArrayBuffer("read",2,3,sucess,fail,complete);
binding.access("access",sucess,fail,complete);
binding.mkdir("mkdir",false,sucess,fail,complete);
binding.rmdir("rmdir",false,sucess,fail,complete);