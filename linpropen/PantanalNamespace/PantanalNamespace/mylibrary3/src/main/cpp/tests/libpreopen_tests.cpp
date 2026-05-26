//
// Created by 80244960 on 2022/12/14.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <gtest/gtest.h>
#include <dirent.h>
#include<fcntl.h>
#include <stdio.h>
#include <libseqmap.h>
#include <libpreopen.h>


using namespace std;
class LibpreopenTests : public ::testing::Test {
public:
    LibpreopenTests() = default;
    ~LibpreopenTests() override = default;


    int WriteInfoToFile(char *str_file, char *str_write_info);

    int ReadInfoFromFile(char *str_path, char *str_read_info);

    int ReadFile(char *str_path);

    int UnlinkFile(uint64_t map_id, const char *str_path, const char *real_path);

    int LstatFile(uint64_t map_id, const char *real_path);

    void SetUp() override {
        uint64_t map_id = 1000;
        char *virtual_path = "/virtualC/test.txt";
        char *real_path = "/data/local/tmp/ns/testc/test.txt";
        char *write_info = "testInfo";
        char *rename_path = "/virtualC/b.txt";
        cout<<"1222222222222222222222"<<endl;

        mkdir("/data/local/tmp/ns/testa", 0755);
        mkdir("/data/local/tmp/ns/testb", 0755);
        mkdir("/data/local/tmp/ns/testc", 0755);
        mkdir("/data/local/tmp/ns/testdelete", 0755);
        mkdir("/data/local/tmp/ns/testinsert", 0755);
        mkdir("/data/local/tmp/ns/testinsert1", 0755);
        Seqmap::GetInstance().InsertMap(map_id, "/data/local/tmp/ns/testc", "/virtualC");
        map_id = 2000;
        Seqmap::GetInstance().InsertMap(map_id, "/data/local/tmp/ns/testb", "/virtualB");
        map_id = 1000;

        WriteInfoToFile(real_path, write_info);
    }
};

int LibpreopenTests::WriteInfoToFile(char *str_file, char *str_write_info) {
    FILE *file_fp;
    file_fp = fopen(str_file, "w");			// 只写的方式打开文件

    if(file_fp == nullptr) {
        perror("fopen");				// 文件打开失败，打印错误信息
        return -1;
    }
    fprintf(file_fp, "%s", str_write_info);
    fclose(file_fp);
    // 关闭文件
    return 0;
}

int LibpreopenTests::ReadInfoFromFile(char *str_path, char *str_read_info) {
    FILE *file_fp;
    file_fp = fopen(str_path, "r+");			// 只写的方式打开文件

    if(file_fp == nullptr) {
        perror("fopen");				// 文件打开失败，打印错误信息
        return -1;
    }
    fgets(str_read_info, 255, file_fp);
    fclose(file_fp);
    // 关闭文件
    return 0;
}

int LibpreopenTests::ReadFile(char *str_path) {
    char *write_info = "testInfo";
    char *str_Info = (char *)malloc(255);
    ReadInfoFromFile(str_path, str_Info);
    int num_str = strcmp(write_info, str_Info);

    if(num_str != 0) {
        return -1;
    }
    delete str_Info;

    return 0;
}

// 测试_open
TEST_F(LibpreopenTests, OpenFile) {

    // 成功打开已初始化的目录下的文件
    uint64_t map_id = 1000;
    char *virtual_path = "/virtualC/test.txt";
    char *real_path = "/data/local/tmp/ns/testc/test.txt";
    EXPECT_LT(0, open_(map_id, virtual_path, O_RDWR));
    EXPECT_EQ(0, ReadFile(real_path));
    // 测试未初始化的目录,该目录未作映射，暂时写死
    ASSERT_GE(0, open_(map_id, "/data/local/tmp/ns/testa/a.txt", O_RDWR));

}

//测试_access
TEST_F(LibpreopenTests, AccessFile) {

    // 判断已初始化的目录下文件存在
    uint64_t map_id = 1000;
    char *virtual_path = "/virtualC/test.txt";
    char *real_path = "/data/local/tmp/ns/testc/test.txt";
    EXPECT_LT(0, open_(map_id, virtual_path, O_RDWR));
    EXPECT_EQ(0, access_(map_id, virtual_path, F_OK));
    // 测试未初始化的目录,该目录未作映射，暂时写死
    ASSERT_EQ(-1, access_(map_id, "/data/local/tmp/ns/testa/a.txt", F_OK));

}

// 测试Rename
TEST_F(LibpreopenTests, RenameFile) {

    // 测试已初始化目录下文件
    uint64_t map_id = 1000;
    char *virtual_path = "/virtualC/test.txt";
    char *rename_path = "/virtualC/b.txt";
    EXPECT_EQ(0, rename_(map_id, virtual_path, rename_path));
    // 测试未初始化的目录,该目录未作映射，暂时写死
    ASSERT_EQ(-1, rename_(map_id, "/data/local/tmp/ns/testa/a.txt", "/data/local/tmp/ns/testa/b.txt"));

}

// 测试lstat
int LibpreopenTests::LstatFile(uint64_t id, const char *real_path) {
    int num_str = 0;
    struct stat st_str;
    int count_str = 0;
    uint64_t map_id = 1000;
    char *virtual_path = "/virtualC/test.txt";
    char *write_info = "testInfo";
    count_str = strlen(write_info);
    num_str = open_(id, real_path, O_RDWR);

    if(num_str <= 0) {
        return -1;
    }
    num_str = lstat_(id, virtual_path, &st_str);

    if(num_str < 0) {
        return -1;
    }
    else {

        if(st_str.st_size == count_str) {
            // 比较文件大小和写入字符串的长度
            return 0;
        }
    }

    return -1;
}

TEST_F(LibpreopenTests, LstatFile) {

    //测试已初始化目录的文件
    uint64_t map_id = 1000;
    char *virtual_path = "/virtualC/test.txt";
    EXPECT_EQ(0, LstatFile(map_id, virtual_path));
    // 测试未初始化的目录,该目录未作映射，暂时写死
    ASSERT_EQ(-1, LstatFile(map_id, "/data/local/tmp/ns/testa/a.txt"));

}

// 测试unlink
int LibpreopenTests::UnlinkFile(uint64_t map_id, const char *str_path, const char *real_path) {
    int num_str = 0;
    num_str = unlink_(map_id, str_path);

    if(num_str < 0 ) {
        return -1;
    }
    num_str = access(real_path, F_OK);

    if (num_str < 0) {
        return 0;
    }

    return -1;
}

TEST_F(LibpreopenTests, UnlinkFile) {

    // 测试通过
    uint64_t map_id = 1000;
    char *virtual_path = "/virtualC/test.txt";
    char *real_path = "/data/local/tmp/ns/testc/test.txt";
    ASSERT_EQ(0, UnlinkFile(map_id, virtual_path, real_path));

}

// 测试InsertMap
TEST_F(LibpreopenTests, InsertMap) {

    uint64_t map_id = 1000;
    ASSERT_EQ(0, Seqmap::GetInstance().InsertMap(map_id, "/data/local/tmp/ns/testinsert", "/testinsert"));
    ASSERT_EQ(0, Seqmap::GetInstance().InsertMap(map_id, "/data/local/tmp/ns/testinsert1", "/testinsert1"));
    map_id = 2000;
    ASSERT_EQ(0, Seqmap::GetInstance().InsertMap(map_id, "/data/local/tmp/ns/testdelete", "/testdelete"));
    //test用作验证插入成功
    ASSERT_LE(0, open_(1000, "/testinsert", 0));

}

// 测试DeleteMap
TEST_F(LibpreopenTests, DeleteMap) {

    uint64_t map_id = 1000;
    ASSERT_EQ(0, Seqmap::GetInstance().InsertMap(map_id,"/data/local/tmp/ns/testdelete", "/testdelete"));
    ASSERT_EQ(0, Seqmap::GetInstance().DeleteMap(map_id));
    
}
