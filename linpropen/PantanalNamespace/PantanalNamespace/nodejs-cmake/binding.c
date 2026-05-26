#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#define NAPI_EXPERIMENTAL
#include <node_api.h>

// file move 函数
static napi_value MoveFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_string_utf8(env, argv[1], destBuffer, sizeof(destBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);
	  
    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[3];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    size_t iData,iCode;
	
	napi_value FailCode,FailData;
    iData = 202;
	iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[4];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

// file copy函数：
static napi_value CopyFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    status = napi_typeof(env, argv[1], &valueType2);
    assert(status == napi_ok);
	
    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_string_utf8(env, argv[1], destBuffer, sizeof(destBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);
	  
    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[3];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[4];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
    //函数入参
    napi_value argvComplete[2];
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

static napi_value ListFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    char srcBuffer[128];
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    //传操作成功的回调函数：
    napi_value sucess = argv[1];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[2];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[3];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}
// file 取文件函数：
static napi_value GetFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    status = napi_typeof(env, argv[1], &valueType2);
    assert(status == napi_ok);
	
    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;
    bool bResult;
    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_bool(env, argv[1], &bResult);
	assert(status == napi_ok);
	
	  
    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[3];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[4];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
    return NULL;
}

// 删除文件接口
static napi_value DeleteFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    char srcBuffer[128];
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    //传操作成功的回调函数：
    napi_value sucess = argv[1];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[2];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[3];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

// 写文本到文件file.writeText
static napi_value WriteTextFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 9;
    napi_value argv[9];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    char srcBuffer[128];
    char destBuffer[128];
	char encodeBuffer[128];
    size_t copied;
    bool bResult;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_string_utf8(env, argv[1], destBuffer, sizeof(destBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);
	
	status = napi_get_value_string_utf8(env, argv[2], encodeBuffer, sizeof(encodeBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",encodeBuffer);
	
	status = napi_get_value_bool(env, argv[3], &bResult);
	assert(status == napi_ok);
	
    //传操作成功的回调函数：
    napi_value sucess = argv[4];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[5];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[6];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

// 写文本到文件file.writeText
static napi_value WriteArrayBuffer(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 9;
    napi_value argv[9];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    char srcBuffer[128];
    char destBuffer[128];
	size_t iLen;
    size_t copied;
    bool bResult;
    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_string_utf8(env, argv[1], destBuffer, sizeof(destBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);
	
	status = napi_get_value_uint32(env, argv[2], &iLen);
    assert(status == napi_ok);
	
	status = napi_get_value_bool(env, argv[3], &bResult);
	assert(status == napi_ok);
	
    //传操作成功的回调函数：
    napi_value sucess = argv[4];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[5];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[6];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

//从文件中读文本 file.readText

static napi_value ReadText(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    status = napi_typeof(env, argv[1], &valueType2);
    assert(status == napi_ok);
	
    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_string_utf8(env, argv[1], destBuffer, sizeof(destBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);
	  
    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[3];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[4];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

// 从文件中读取文本：file.readArrayBuffer
static napi_value ReadArrayBuffer(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 8;
    napi_value argv[8];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    char srcBuffer[128];
    char destBuffer[128];
	size_t iPos;
	size_t iLen;
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_uint32(env, argv[1], &iPos);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);
	
	status = napi_get_value_uint32(env, argv[2], &iLen);
    assert(status == napi_ok);
	
	
    //传操作成功的回调函数：
    napi_value sucess = argv[3];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[4];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);

    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[5];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

//判断文件目录是否存在：access
// 删除文件接口
static napi_value AccessFile(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    char srcBuffer[128];
    size_t copied;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    //传操作成功的回调函数：
    napi_value sucess = argv[1];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[2];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);

    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[3];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    napi_value argvComplete[2];

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

// 创建目录：mkdir

static napi_value Mkdir(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    status = napi_typeof(env, argv[1], &valueType2);
    assert(status == napi_ok);
	
    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;
    bool bResult;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_bool(env, argv[1], &bResult);
	assert(status == napi_ok);

	  
    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[3];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[4];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);

    //函数入参
    napi_value argvComplete[2];


    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

// 删除目录：rmdir
static napi_value Rmdir(napi_env env, napi_callback_info info) {

    napi_status status;
    //接收源地址和目标地址
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    napi_valuetype valueType1, valueType2;

    status = napi_typeof(env, argv[0], &valueType1);
    assert(status == napi_ok);

    status = napi_typeof(env, argv[1], &valueType2);
    assert(status == napi_ok);
	
    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;
    bool bResult;

    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
	
    status = napi_get_value_bool(env, argv[1], &bResult);
	assert(status == napi_ok);

	  
    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    napi_value result;
    //函数入参
    napi_value strSucess;
    char *ptrSucess = "copy sucess";
	status = napi_create_string_utf8(env, ptrSucess, strlen(ptrSucess), &strSucess);
    assert(status == napi_ok);
    napi_value argvSucess[] = {strSucess};

    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        sucess,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvSucess,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作失败的回调函数：
    napi_value fail = argv[3];   // 第二个参数 function 回调
    //napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
		
    //napi_value result;
    //函数入参
    size_t iData,iCode;
	
    napi_value FailCode,FailData;
    iData = 202;
    iCode = 300;
	
    status = napi_create_int32(env, iData, &FailData);
    assert(status == napi_ok);
	
	status = napi_create_int32(env, iData, &FailCode);
    assert(status == napi_ok);
	
    napi_value argvFail[] = {FailCode,FailData};
    status = napi_create_string_utf8(env, "fail", NAPI_AUTO_LENGTH, argvFail);
    assert(status == napi_ok);
		 
    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        fail,     // js 回调函数句柄
        2,      // js 回调函数接受参数个数
        argvFail,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);
	
    //传操作完成的回调函数：
    napi_value complete = argv[4];   // 第二个参数 function 回调
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);

    //函数入参
    napi_value argvComplete[2];


    status = napi_call_function( // 调用 js 回调函数
        env,    // 当前程序执行上下文
        global, // js 回调的 this 对象，在 js 回调中可以验证: console.log(this === global); // true
        complete,     // js 回调函数句柄
        1,      // js 回调函数接受参数个数
        argvComplete,   // js 回调函数参数数组
        &result // js 回调函数中如果有 retrun，将会被 result 接受到
    );
    assert(status == napi_ok);

    return NULL;
}

#define DECLARE_NAPI_METHOD(name, func)                                        \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

static napi_value Init(napi_env env, napi_value exports) {
    napi_status status;
    napi_property_descriptor desc = DECLARE_NAPI_METHOD("copy", CopyFile);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);
  
    napi_property_descriptor descMove = DECLARE_NAPI_METHOD("move", MoveFile);
    status = napi_define_properties(env, exports, 1, &descMove);
    assert(status == napi_ok);
	
	napi_property_descriptor descList = DECLARE_NAPI_METHOD("list", ListFile);
    status = napi_define_properties(env, exports, 1, &descList);
    assert(status == napi_ok);
	
    napi_property_descriptor descGet = DECLARE_NAPI_METHOD("get", GetFile);
    status = napi_define_properties(env, exports, 1, &descGet);
    assert(status == napi_ok);
	
    napi_property_descriptor descDelete = DECLARE_NAPI_METHOD("delete", DeleteFile);
    status = napi_define_properties(env, exports, 1, &descDelete);
    assert(status == napi_ok);
	
    napi_property_descriptor descWrite = DECLARE_NAPI_METHOD("writeText", WriteTextFile);
    status = napi_define_properties(env, exports, 1, &descWrite);
    assert(status == napi_ok);
	
    napi_property_descriptor descWriteArray = DECLARE_NAPI_METHOD("writeArrayBuffer", WriteArrayBuffer);
    status = napi_define_properties(env, exports, 1, &descWriteArray);
    assert(status == napi_ok);
	
    napi_property_descriptor descRead = DECLARE_NAPI_METHOD("readText", ReadText);
    status = napi_define_properties(env, exports, 1, &descRead);
    assert(status == napi_ok);

    napi_property_descriptor descReadArray = DECLARE_NAPI_METHOD("readArrayBuffer", ReadArrayBuffer);
    status = napi_define_properties(env, exports, 1, &descReadArray);
    assert(status == napi_ok);
	
    napi_property_descriptor descAccess = DECLARE_NAPI_METHOD("access", AccessFile);
    status = napi_define_properties(env, exports, 1, &descAccess);
    assert(status == napi_ok);
	
	napi_property_descriptor descMkdir = DECLARE_NAPI_METHOD("mkdir", Mkdir);
    status = napi_define_properties(env, exports, 1, &descMkdir);
    assert(status == napi_ok);
	
	napi_property_descriptor descRmdir = DECLARE_NAPI_METHOD("rmdir", Rmdir);
    status = napi_define_properties(env, exports, 1, &descRmdir);
    assert(status == napi_ok);
	
	return exports;
	
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

