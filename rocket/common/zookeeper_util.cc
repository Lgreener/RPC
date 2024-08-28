#include "rocket/common/zookeeper_util.h"
#include "rocket/common/log.h"

#include <cstring>
#include <semaphore.h>
#include <iostream>
#include <ctime>  

namespace rocket {

void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
		{
			sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
		}
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{

}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源  MySQL_Conn
		m_zhandle = nullptr;  // 防止重复释放
    }
}

void ZkClient::Start()
{
    std::string host = "127.0.0.1";
    std::string port =  "2181";
    std::string connstr = host + ":" + port;
    
	/*
	zookeeper_mt：多线程版本
	zookeeper的API客户端程序提供了三个线程
	API调用线程 
	网络I/O线程  pthread_create  poll
	watcher回调线程 pthread_create
	*/
    //异步
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle) 
    {
        ERRORLOG("zookeeper init error!");
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);
    DEBUGLOG("zookeeper init success!");
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			DEBUGLOG("znode create success, path:[%s]", path);
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			ERRORLOG("znode create error, path:[%s], flag:[%d]", path, flag);
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
	struct String_vector children;;
	int flag = zoo_get_children(m_zhandle, path, 0, &children);
	DEBUGLOG("num:%d", children.count)

	if (flag != ZOK)
	{
		ERRORLOG("failed to get method address find, path:[%s], flag:[%d]", *path, flag);
		return "";
	}
	else
	{
		if(children.count<=0){
			DEBUGLOG("no method address, method address number:[%d]", children.count);
			return "";
		}
		std::string path_str(path);

		// 随机选择负载均衡
    	std::srand(std::time(0)); // Mersenne Twister 随机数生成器
		// 定义均匀分布，范围是 [0, children.count - 1]
    	int chooseIndex = std::rand() % children.count;  // 生成随机数，作为选择的索引

		DEBUGLOG("path:[%s],children.count:%d,chooseIndex:%d", path, children.count, chooseIndex)
		// 选中的子节点的字符串
        char* selectedChild = children.data[chooseIndex];
        size_t dataLen = strlen(selectedChild);

		// 确保buffer足够容纳字符串数据
        if (dataLen >= sizeof(buffer)) 
        {
            ERRORLOG("Data length exceeds buffer size, path:[%s], data:[%s]", path, selectedChild);
            return "";
        }

		memcpy(&buffer[0], &selectedChild[0], dataLen);

		return buffer;
	}
}

}