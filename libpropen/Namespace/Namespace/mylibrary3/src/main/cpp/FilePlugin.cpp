#include <cstdio>
#include <cstring>

#include <napi/native_api.h>
#include <napi/native_node_api.h>

#include <namespace/NamespaceLibc.h>
#include <Log.h>



#include <libpreopen.h>

#include <libseqmap.h>

#include <libc_wrapper.h>

// file move 函数
static napi_value MoveFile(napi_env env, napi_callback_info info) {
    napi_status status;
    //接收源地址和目标地址
    int n = 5;
    __android_log_print(ANDROID_LOG_INFO, "ProjectName", "66666666666/n");
    size_t argc = 5;
    napi_value argv[5];
    status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    assert(status == napi_ok);

    char srcBuffer[128];
    char destBuffer[128];
    size_t copied;
    //__android_log_print(ANDROID_LOG_INFO, LOG_TAG,"array:  ArrayBuffer (ie: bytes=%d)", (int)byte_length1);
    status =  napi_get_value_string_utf8(env, argv[0], srcBuffer, sizeof(srcBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",srcBuffer);
    __android_log_print(ANDROID_LOG_INFO, "ProjectName", "srcBuffer = %s",srcBuffer);

    status = napi_get_value_string_utf8(env, argv[1], destBuffer, sizeof(destBuffer), &copied);
    assert(status == napi_ok);
    printf("srcBuffer==%s\n",destBuffer);

    __android_log_print(ANDROID_LOG_INFO, "ProjectName", "destBuffer = %s",destBuffer);

    //传操作成功的回调函数：
    napi_value sucess = argv[2];   // 第二个参数 function 回调
    napi_value global;
    status = napi_get_global(env, &global);   // 获取当前执行 js 的 global 对象
    assert(status == napi_ok);
    uint64_t map_id;
    map_id = 1000;
    Seqmap::GetInstance().InsertMap(map_id, "/data/local/tmp/ns/testdelete", "/testdelete");

    int id = open_(map_id, "/testdelete", O_RDWR);

    __android_log_print(ANDROID_LOG_INFO, "ProjectName", "testdelete =%s",id);
  //  int fd = _open("/data/data/com.test.ule.uleplugindemo", O_RDONLY);

    return NULL;
}

static napi_value JSInit(napi_env env, napi_callback_info info) {
    return nullptr;
}

static napi_value FileExport(napi_env env, napi_value exports) {
    static napi_property_descriptor desc[] = {
            DECLARE_NAPI_FUNCTION("init", JSInit),
            DECLARE_NAPI_FUNCTION("move", MoveFile),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module fileModule = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = FileExport,
        .nm_modname = "ule_file",
        .nm_priv = ((void *) nullptr),
        .reserved = {nullptr}
};

extern "C" __attribute__((constructor)) void FileRegister() {
    LOG(INFO) << "FileRegister constructor";

    napi_module_register(&fileModule);
}