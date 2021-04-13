#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <time.h>
#include <algorithm>
#include <assert.h>
#include <set>
#include <omp.h>
#include<math.h>
using namespace std;

ofstream outfile("output_end.txt", ios::trunc);

//可调参数

int server_choose_times =1;
int a_bound = 10000000;//请求数小于这个值时，使用爬山算法
int max_open_list_size = 6000;  //越小规划越快,越大规划地越好

float value_piancha;//弥补普遍核数比内存数少的量纲问题，认为核数量纲为内存的v_p倍
float piancha_size = 0;//弥补尺寸越大，灵活性越小的问题。

//灵魂调参点
float piancha_run_purchse = 3; //越大purchase越重要
float purchase_wt= 8;         //越大购买价格更重要
float vl_wt = 1.5;  //增大核数内存量纲比
float vl_wt2 = 1.5;//用在部署的量纲扩张系数

class IntIntInt
{
public:
    IntIntInt(int vm_id,int server_id,int node)
    {
        this ->m_vmid = vm_id;
        this ->m_serverid = server_id;
        this ->m_node = node;
    }
    int m_vmid;
    int m_serverid;
    int m_node;
};

class IntInt
{
public:
    IntInt(int id,int node)
    {
        this ->m_id = id;
        this ->m_node = node;
    }
    int m_id;
    int m_node;
};
class StrInt
{
public:
    StrInt(string Type,int num)
    {
        this ->m_Type = Type;
        this ->m_num = num;
    }
    string m_Type;
    int m_num;
};
class StrFloat
{
public:
    StrFloat(string Type,float num)
    {
        this ->m_Type = Type;
        this ->m_num = num;
    }
    string m_Type;
    float m_num;
};
class SFI
{
public:
    SFI(string Type,float num1,int num2,double rate)
    {
        this ->m_Type = Type;
        this ->m_num1 = num1;
        this ->m_num2 = num2;
	this ->m_num3 = rate;
    }
    string m_Type;
    float m_num1;
    int m_num2;
    double m_num3;
};
class SFII
{
public:
    SFII(string Type,float num1,int num2,int num3)
    {
        this ->m_Type = Type;
        this ->m_num1 = num1;
        this ->m_num2 = num2;
        this ->m_num3 = num3;
    }
    string m_Type;
    float m_num1;
    int m_num2;
    int m_num3;
};

class SFIII
{
public:
    SFIII(string Type,float num1,int num2,int num3,int num4)
    {
        this ->m_Type = Type;
        this ->m_num1 = num1;
        this ->m_num2 = num2;
        this ->m_num3 = num3;
        this ->m_num4 = num4;
    }
    string m_Type;
    float m_num1;
    int m_num2;
    int m_num3;
    int m_num4;
};


class FIII
{
public:
    FIII(float num1,int num2,int num3,int num4)
    {
        this ->m_F = num1;
        this ->m_vmid = num2;
        this ->m_serverid = num3;
        this ->m_oper = num4;
    }
    float m_F;
    int m_vmid;
    int m_serverid;
    int m_oper;
};

class FVIII  //{F,v{ {serverid or purchase_num,oper}...... }}
{
    public:
    FVIII(float F,vector<IntIntInt> OP)
    {
        this ->m_F = F;
        
        this ->m_OP = OP;
    }
    float m_F;
    vector<IntIntInt> m_OP;
};

class LongInt
{
public:
    LongInt( long long id,int num)
    {
        this ->m_id = id;
        this ->m_num = num;
    }
    long long  m_id;
    int m_num;
};

class compareFVIII
{
    public:
    bool operator()(const FVIII&fv1 ,const FVIII&fv2) const
    {
        // 排序方式为升序

        if(fv1.m_F !=fv2.m_F)                                       //先尝试按F值升序排列
        {return fv1.m_F <fv2.m_F;}
        else if(fv1.m_OP.size()!=fv2.m_OP.size())                    //如果F值相同，则尺寸大的排前面                       
        {return fv1.m_OP.size() >fv2.m_OP.size();}
        else if(fv1.m_OP.back().m_serverid!=fv2.m_OP.back().m_serverid)     //如果F，尺寸都相同，则服务器id靠前的排前面（注意purchase时id为0）
        {return fv1.m_OP.back().m_serverid<fv2.m_OP.back().m_serverid;} 
        else if(fv1.m_OP.back().m_node!=fv1.m_OP.back().m_node)    //如果F，尺寸,服务器id都相同，则操作数小的排前面（这会导致优先塞入A节点）
        {return fv1.m_OP.back().m_node<fv2.m_OP.back().m_node;}  
        else                                                    //如果F，尺寸,服务器id,操作数都相同，按vm_id排序
        {return fv1.m_OP.back().m_vmid<fv2.m_OP.back().m_vmid;}  
               
        
    }
};

class compareFIII
{
    public:
    bool operator()(const FIII&fiii1 ,const FIII&fiii2) const
    {
        // 排序方式为升序

        if(fiii1.m_F !=fiii2.m_F)                                       //先尝试按F值升序排列
        return fiii1.m_F <fiii2.m_F;
        else if(fiii1.m_serverid!=fiii2.m_serverid)                    //如果F值相同，则按服务器id升序排名                       
        return fiii1.m_serverid<fiii2.m_serverid;
        else                                                    //如果F，服务器id都相同，按vm_id排序
        return fiii1.m_vmid<fiii2.m_vmid; 
               
        
    }
};


class compareStrInt
{
public:
    bool operator()(const StrInt&si1 ,const StrInt&si2) const
    {
        // 排序方式为降序
        return si1.m_num >si2.m_num;
    }
};
class compareStrFloat
{
public:
    bool operator()(const StrFloat&sf1 ,const StrFloat&sf2) const
    {
        // 排序方式为升序
        return sf1.m_num <sf2.m_num;
    }
};
class compareSFI
{
public:
    bool operator()(const SFI&sfi1 ,const SFI&sfi2) const
    {
        // 排序方式为升序
        if(sfi1.m_num1 !=sfi2.m_num1)
        return sfi1.m_num1 <sfi2.m_num1;
        else
        return sfi1.m_num3>sfi2.m_num3;

    }
};
class compareSFII
{
public:
    bool operator()(const SFII&sfii1 ,const SFII&sfii2) const
    {
        // 排序方式为升序
        return sfii1.m_num1 <sfii2.m_num1;
    }
};
class compareSFIII
{
public:
    bool operator()(const SFIII&sfiii1 ,const SFIII&sfiii2) const
    {
        // 排序方式为升序
        return sfiii1.m_num1 <sfiii2.m_num1;
    }
};




// 服务器信息
unordered_map<string,vector<int>> server_type_map;
// 虚拟机信息
unordered_map<string,vector<int>> vm_type_map;
// 请求信息
vector<vector<string>> requestInfos;
// 已有服务器信息
unordered_map<int,vector<int>> servers_info_map;
// 已有虚拟机数值信息
unordered_map<int,vector<int>> vms_info_map;
//服务器类型运行成本表
set<SFI,compareSFI> serverTypeCostSet;
set<SFI,compareSFI> serverTypeCostSet_today;
//虚拟机类型大小表
set<StrInt,compareStrInt> vmTypeSizeSet;
//已部署虚拟机信息
unordered_map<string,string> vmIdType_map;
//已购买服务器编号和类型信息
unordered_map<int,string> serverIdType_map;
//全部请求信息
unordered_map<int,vector<vector<int>>> reqs_map;

// 高性价比服务器列表
set<SFIII,compareSFIII> purchase_type_set;

//每天的迁移指令表
vector<IntIntInt> mig_list;


// 每台服务器里的虚拟机信息
unordered_map<int,vector<vector<int>>> server_vminfo_map;  //severid:{{vm_id,cores,mems,0orAorB},......}

//A*算法开集组件
// set<FVIII,compareFVIII> open_list;
vector<IntIntInt> oper_list_copy;
set<FIII,compareFIII> open_list_cb;
vector<FIII> oper_list_cb;

vector<IntIntInt> oper_list_scti;
set<FVIII,compareFVIII> open_list_sct;
vector<FIII> oper_list_sctf;
FVIII oper_list_sct = {0.0,{{0,0,0}}};

string best_server_type;
int max_need_mem=0;
int max_need_core =0;
int server_id_count=0;

int purchase_type_num = 0; //定义购买请求的类型数
int server_num =0;
int vm_num=0;

int max_core_mem=0;
int max_mem_core=0;
int total_day;
int today;
int first_read_lenth;

int all_vmcores = 0;
int all_vmmem =0;
int max_vmcores = 0;
int max_vmmem =0;

int migration_num=0;
int migration_num_max=0;
long long SERVERCOST = 0,POWERCOST=0,TOTALCOST =0;
clock_t start, finish;

int purchase_num_climb;

int str2int(string s)
{
    int num;
    stringstream ss(s);
    ss>>num;
    return num;
}
string int2str(int num)
{
    string s;
    stringstream ss;
    ss<<num;
    ss>>s;
    return s;
}


void generateVm(string vmType,string vmCpuCores,string vmMemory,string vmTwoNodes){
    string _vmType ;


    _vmType = vmType.substr(1,vmType.size()-2);

    int _vmCpuCores = 0,_vmMemory=0,_vmTwoNodes=0;

    _vmCpuCores = str2int(vmCpuCores.substr(0,vmCpuCores.size()-1));

    _vmMemory = str2int(vmMemory.substr(0,vmMemory.size()-1));

    _vmTwoNodes = str2int(vmTwoNodes.substr(0,vmTwoNodes.size()-1));



    vm_type_map[_vmType] = vector<int>{_vmCpuCores,_vmMemory,_vmTwoNodes};
    int _vmSize = _vmCpuCores +_vmMemory + _vmTwoNodes*10000; //计算vm大小，使双节点恒大于单节点，先部署大
    vmTypeSizeSet.insert(StrInt(_vmType,_vmSize));

}

void generateServer(string serverType,string cpuCores,string memorySize,string serverCost,string powerCost){
    string _serverType;

    _serverType = serverType.substr(1,serverType.size()-2);

    int _cpuCores=0,_memorySize=0,_serverCost=0,_powerCost=0;
    _cpuCores = str2int(cpuCores.substr(0,cpuCores.size()-1));
    _memorySize = str2int(memorySize.substr(0,memorySize.size()-1));
    _serverCost = str2int(serverCost.substr(0,serverCost.size()-1));
    _powerCost = str2int(powerCost.substr(0,powerCost.size()-1));

    // 更新服务器类型表和服务器类型性价比排序表
    server_type_map[_serverType] = vector<int>{_cpuCores,_memorySize,_serverCost,_powerCost};


}
void del_vm(int server_id,int vm_id ,int vm_node,int vm_core,int vm_mem)
{
    if(vm_node==0)
    {
        // set_vm_map[today].push_back(IntInt(-1,-1)); //表示无需操作
        auto it_temp = vms_info_map.find(vm_id);
        if(it_temp !=vms_info_map.end())
        {vms_info_map.erase(it_temp);}//删除虚拟机信息
        servers_info_map[server_id][0] += vm_core/2;
        servers_info_map[server_id][1] += vm_core/2;
        servers_info_map[server_id][2] += vm_mem/2;
        servers_info_map[server_id][3] += vm_mem/2;
        servers_info_map[server_id][4] -= vm_core;
        servers_info_map[server_id][5] -= vm_mem;
        for(auto it_temp2 =server_vminfo_map[server_id].begin();it_temp2!=server_vminfo_map[server_id].end();it_temp2++)
        {   
            // cout<<"看看指针取了个啥出来："<<(*it_temp2)[0]<<endl;
            if((*it_temp2)[0]==vm_id)
            {server_vminfo_map[server_id].erase(it_temp2);
            break;} //删除服务器虚拟机信息
        }

    }
    if(vm_node==1)
    {
        // set_vm_map[today].push_back(IntInt(-1,-1)); //表示无需操作
        auto it_temp = vms_info_map.find(vm_id);
        if(it_temp !=vms_info_map.end())
        {vms_info_map.erase(it_temp);}//删除虚拟机信息
        servers_info_map[server_id][0] += vm_core;
        
        servers_info_map[server_id][2] += vm_mem;
        servers_info_map[server_id][4] -= vm_core;
        servers_info_map[server_id][5] -= vm_mem;
        
        for(auto it_temp2 =server_vminfo_map[server_id].begin();it_temp2!=server_vminfo_map[server_id].end();it_temp2++)
        {   
            // cout<<"看看指针取了个啥出来："<<(*it_temp2)[0]<<endl;
            if((*it_temp2)[0]==vm_id)
            {server_vminfo_map[server_id].erase(it_temp2);
            break;} //删除服务器虚拟机信息
        }

    }
    if(vm_node==2)
    {
        // set_vm_map[today].push_back(IntInt(-1,-1)); //表示无需操作
        auto it_temp = vms_info_map.find(vm_id);
        if(it_temp !=vms_info_map.end())
        {vms_info_map.erase(it_temp);}//删除虚拟机信息
        servers_info_map[server_id][1] += vm_core;
        
        servers_info_map[server_id][3] += vm_mem;
        servers_info_map[server_id][4] -= vm_core;
        servers_info_map[server_id][5] -= vm_mem;
        for(auto it_temp2 =server_vminfo_map[server_id].begin();it_temp2!=server_vminfo_map[server_id].end();it_temp2++)
        {   
            // cout<<"看看指针取了个啥出来："<<(*it_temp2)[0]<<endl;
            if((*it_temp2)[0]==vm_id)
            {server_vminfo_map[server_id].erase(it_temp2);
            break;} //删除服务器虚拟机信息
        }

    }
    vm_num--;
}


void migration()
{
    int mig_num = 0;
    int max_mig_num =(int) vms_info_map.size()*3/100 ;
    // migration_num_max += max_mig_num;
    max_mig_num -= 1;
    int key = 1;
    int num_point=5;       //-----------参数可调
    int num=1;
    while( mig_num<max_mig_num && num<num_point){
    num++;
    for(int i = server_id_count-1;i>=0;i-- )//从后往前遍历服务器
    {
	int server_vm_size = server_vminfo_map[i].size();

        if(server_vm_size<num && server_vm_size>0)   //如果服务器不是空的
        {
            if(mig_num<max_mig_num)
            {
                for(int j=0;j<server_vminfo_map[i].size();j++)  //遍历这个服务器里所有的虚拟机
                {
                    if(mig_num>=max_mig_num)
                    {break;}
                    int vm_id = server_vminfo_map[i][j][0];
                    int vm_core = server_vminfo_map[i][j][1];
                    int vm_mem = server_vminfo_map[i][j][2];
                    int node = server_vminfo_map[i][j][3];
                    for(int k =0;k<server_id_count;k++)      //从bound开始遍历所有服务器
                    {
                        int serverk_size = server_vminfo_map[k].size();
                        if(k!=i&&serverk_size!=0)
                        {
                        if(node==0)//如果虚拟机为双节点
                        {
                            if(servers_info_map[k][0]>=vm_core/2 && servers_info_map[k][1]>=vm_core/2 && servers_info_map[k][2]>=vm_mem/2 && servers_info_map[k][3]>=vm_mem/2)
                            {
                                mig_list.push_back(IntIntInt(vm_id,k,0));
                                del_vm(i,vm_id,node,vm_core,vm_mem);
                                vms_info_map[vm_id]=vector<int>{vm_core,vm_mem,k,0};
                                servers_info_map[k][0] -= vm_core/2;
                                servers_info_map[k][1] -= vm_core/2;
                                servers_info_map[k][2] -= vm_mem/2;
                                servers_info_map[k][3] -= vm_mem/2;
                                servers_info_map[k][4] += vm_core;
                                servers_info_map[k][5] += vm_mem;
                                server_vminfo_map[k].push_back(vector<int>{vm_id,vm_core,vm_mem,0});
                                mig_num++;
                                break;

                            }
                        }
                        else if(node==1) //如果虚拟机布置在A
                        {
                            if(servers_info_map[k][0]>=vm_core&&servers_info_map[k][2]>=vm_mem)
                            {
                                del_vm(i,vm_id,node,vm_core,vm_mem);
                                mig_list.push_back(IntIntInt(vm_id,k,1));
                                vms_info_map[vm_id]=vector<int>{vm_core,vm_mem,k,1};
                                servers_info_map[k][0] -= vm_core;
                                servers_info_map[k][2] -= vm_mem;
                                servers_info_map[k][4] += vm_core;
                                servers_info_map[k][5] += vm_mem;
                                server_vminfo_map[k].push_back(vector<int>{vm_id,vm_core,vm_mem,1});
                                mig_num++;
                                break;
                            }

                        }
                        else if(node==2) //如果虚拟机布置在B
                        {
                            if(servers_info_map[k][1]>=vm_core&&servers_info_map[k][3]>=vm_mem)
                            {
                                del_vm(i,vm_id,node,vm_core,vm_mem);
                                mig_list.push_back(IntIntInt(vm_id,k,2));
                                vms_info_map[vm_id]=vector<int>{vm_core,vm_mem,k,2};
                                servers_info_map[k][1] -= vm_core;
                                servers_info_map[k][3] -= vm_mem;
                                servers_info_map[k][4] += vm_core;
                                servers_info_map[k][5] += vm_mem;
                                server_vminfo_map[k].push_back(vector<int>{vm_id,vm_core,vm_mem,2});
                                mig_num++;
                                break;
                            }

                        }
                        }

                    }
                }
		
            }
        }
        
    }
    }


    // 如果循环后还有迁移次数，继续迁移，迁移前面核数大于内存以及内存大于核数的情况
    key = 1;
    double Rate_err=0.2;       //-----------参数可调
    int Vast_vmid=0;
    int Vast_vmcpu=0;
    int Vast_vmmemory=0;
    int Vast_vmnode=0;
    double cpuRate=0.0;
    double memoryRate=0.0;

    while( mig_num<max_mig_num && key == 1)
    {
	key=0;
    	for(int i=0;i<server_id_count;i++)//从前往后遍历服务器
    	{
	    int server_vm_size = server_vminfo_map[i].size();
	    if(mig_num>=max_mig_num)
            {break;}
	    if( server_vm_size>0 )
	    {
		int point=0;
	    	string Type = serverIdType_map[i];
	    	vector<int> _serverInfo = server_type_map[Type];
	    	vector<int> _useserver = servers_info_map[i];
	    	cpuRate = 1 - double(_useserver[0]+_useserver[1])/double(_serverInfo[0]);
	    	memoryRate = 1 - double(_useserver[2]+_useserver[3])/double(_serverInfo[1]);

		if((cpuRate-memoryRate)>Rate_err)
	    	{
		    point = 1;
		}
		if(cpuRate<0.75 && memoryRate<0.75 && cpuRate<memoryRate)   //-----------参数可调
	    	{
		    point = 1;
		}
		if((memoryRate-cpuRate)>Rate_err)
		{
		    point = -1;
		}
		if(cpuRate<0.75 && memoryRate<0.75 && cpuRate>memoryRate)   //-----------参数可调
	    	{
		    point = -1;
		}
		if(point==0)
		{
		    continue;
		}
		Vast_vmid = 0; 
		Vast_vmcpu = 0;
		Vast_vmmemory = 0;
		Vast_vmnode = 0;
	    	for(int j=0;j<server_vm_size;j++)  //遍历这个服务器里所有的虚拟机，找到服务器内cpu核数最多的虚拟机
                {
		    int vm_id = server_vminfo_map[i][j][0];
                    int vm_core = server_vminfo_map[i][j][1];
                    int vm_mem = server_vminfo_map[i][j][2];
                    int node = server_vminfo_map[i][j][3];
		    if(vm_core>Vast_vmcpu && point==1)
		    {
			Vast_vmid = vm_id; 
			Vast_vmcpu = vm_core;
			Vast_vmmemory = vm_mem;
			Vast_vmnode = node;
		    }
		    if(vm_mem>Vast_vmmemory && point==-1)
		    {
			Vast_vmid = vm_id; 
			Vast_vmcpu = vm_core;
		        Vast_vmmemory = vm_mem;
			Vast_vmnode = node;
		    }
		}
		for(int k =i+1;k<server_id_count;k++)      
                {
		    if(server_vminfo_map[k].size()==0)
		    {continue;}
		    Type = serverIdType_map[k];
	    	    _serverInfo = server_type_map[Type];
	    	    _useserver = servers_info_map[k];
	    	    cpuRate = 1 - double(_useserver[0]+_useserver[1])/double(_serverInfo[0]);
	    	    memoryRate = 1 - double(_useserver[2]+_useserver[3])/double(_serverInfo[1]);
		    if(cpuRate<0.3 && memoryRate<0.3 && point==1)    //-----------参数可调
		    {
		        break;
		    }
		    if(cpuRate<0.1 && memoryRate<0.1 && point==-1)    //-----------参数可调
		    {
			break;
		    }
		    if(Vast_vmnode==0)//如果虚拟机为双节点
                    {
                       if(servers_info_map[k][0]>=Vast_vmcpu/2 && servers_info_map[k][1]>=Vast_vmcpu/2 && servers_info_map[k][2]>=Vast_vmmemory/2 && servers_info_map[k][3]>=Vast_vmmemory/2)
                       {
                           mig_list.push_back(IntIntInt(Vast_vmid,k,0));
                           del_vm(i,Vast_vmid,Vast_vmnode,Vast_vmcpu,Vast_vmmemory);
                           vms_info_map[Vast_vmid]=vector<int>{Vast_vmcpu,Vast_vmmemory,k,0};
                           servers_info_map[k][0] -= Vast_vmcpu/2;
                           servers_info_map[k][1] -= Vast_vmcpu/2;
                           servers_info_map[k][2] -= Vast_vmmemory/2;
                           servers_info_map[k][3] -= Vast_vmmemory/2;
                           servers_info_map[k][4] += Vast_vmcpu;
                           servers_info_map[k][5] += Vast_vmmemory;
                           server_vminfo_map[k].push_back(vector<int>{Vast_vmid,Vast_vmcpu,Vast_vmmemory,0});
                           mig_num++;
			   key=1;
                           break;
                        }
                    }
                    else if(Vast_vmnode==1) //如果虚拟机布置在A
                    {
                        if(servers_info_map[k][0]>=Vast_vmcpu&&servers_info_map[k][2]>=Vast_vmmemory)
                        {
                            del_vm(i,Vast_vmid,Vast_vmnode,Vast_vmcpu,Vast_vmmemory);
                            mig_list.push_back(IntIntInt(Vast_vmid,k,1));
                            vms_info_map[Vast_vmid]=vector<int>{Vast_vmcpu,Vast_vmmemory,k,1};
                            servers_info_map[k][0] -= Vast_vmcpu;
                            servers_info_map[k][2] -= Vast_vmmemory;
                            servers_info_map[k][4] += Vast_vmcpu;
                            servers_info_map[k][5] += Vast_vmmemory;
                            server_vminfo_map[k].push_back(vector<int>{Vast_vmid,Vast_vmcpu,Vast_vmmemory,1});
                            mig_num++;
			    key=1;
                            break;
                         }
                     }
                     else if(Vast_vmnode==2) //如果虚拟机布置在B
                     {
                         if(servers_info_map[k][1]>=Vast_vmcpu&&servers_info_map[k][3]>=Vast_vmmemory)
                         {
                             del_vm(i,Vast_vmid,Vast_vmnode,Vast_vmcpu,Vast_vmmemory);
                             mig_list.push_back(IntIntInt(Vast_vmid,k,2));
                             vms_info_map[Vast_vmid]=vector<int>{Vast_vmcpu,Vast_vmmemory,k,2};
                             servers_info_map[k][1] -= Vast_vmcpu;
                             servers_info_map[k][3] -= Vast_vmmemory;
                             servers_info_map[k][4] += Vast_vmcpu;
                             servers_info_map[k][5] += Vast_vmmemory;
                             server_vminfo_map[k].push_back(vector<int>{Vast_vmid,Vast_vmcpu,Vast_vmmemory,2});
                             mig_num++;
			     key=1;
                             break;
                         }
                     }
		 }
	     }
	}
    }
}

    while( mig_num<max_mig_num && key == 1){
        key = 0;
        for i in serverIdType_map:
        
}

//计算每天拥有虚拟机的服务器运行成本
void serverPower()
{
    for(int i=0;i<server_id_count;i++)
    {
	if( server_vminfo_map[i].size()>0 )
	{
	    string Type = serverIdType_map[i];
	    vector<int> _serverInfo = server_type_map[Type];
	    POWERCOST += _serverInfo[3];
	}
	else
	{
	    break;
	}
    }
}

// 扩容时找到最佳服务器
string Find_server(int vm_allcore, int vm_allmemory, int vm_maxcore, int vm_maxmem,int sct)
{

    serverTypeCostSet_today.clear();
    int _needvm_core = vm_allcore;
    int _needvm_memory = vm_allmemory;

    int _vm_maxcore = vm_maxcore;
    int _vm_maxmemory = vm_maxmem;
    double vm_maxrate = double(_needvm_core) / double(_needvm_memory);
    double _cmrate=0.0;
    double min=100.0;
    string best_server;
    for(auto it=serverTypeCostSet.begin();it!= serverTypeCostSet.end();it++)
    {
	if(server_type_map[it->m_Type][0]>=_vm_maxcore&&server_type_map[it->m_Type][1]>=_vm_maxmemory)
        {
            if(it->m_num2!=0)
            {	
                string f_type = it->m_Type;
                double f_rate = it->m_num3;
                if(vm_maxrate>=1)
                {
                        if( f_rate < 1 )
                        {
                            _cmrate = 100 + (vm_maxrate -f_rate)/vm_maxrate;
                            _cmrate += purchase_wt*(it->m_num1);
                            serverTypeCostSet_today.insert(SFI(f_type,float(_cmrate),it->m_num2,double(it->m_num1)));
                        }
                        else
                        {
                            if(vm_maxrate<f_rate)
                           { _cmrate =( f_rate -vm_maxrate)/vm_maxrate;
                           _cmrate += purchase_wt*(it->m_num1);
                            serverTypeCostSet_today.insert(SFI(f_type,float(_cmrate),it->m_num2,double(it->m_num1)));}
                            else
                           { _cmrate =( vm_maxrate- f_rate)/vm_maxrate ;
                           _cmrate += purchase_wt*(it->m_num1);
                            serverTypeCostSet_today.insert(SFI(f_type,float(_cmrate),it->m_num2,double(it->m_num1)));}

                        }
                }
                if(vm_maxrate<1)
                {
                        if( f_rate >= 1 )
                        {
                            _cmrate = 100 + ( f_rate -vm_maxrate)/vm_maxrate;
                            _cmrate += purchase_wt*(it->m_num1);
                            serverTypeCostSet_today.insert(SFI(f_type,float(_cmrate),it->m_num2,double(it->m_num1)));
                        }
                        else
                        {
                            if(vm_maxrate<f_rate)
                           { _cmrate =( f_rate -vm_maxrate)/vm_maxrate;
                           _cmrate += purchase_wt*(it->m_num1);
                            serverTypeCostSet_today.insert(SFI(f_type,float(_cmrate),it->m_num2,double(it->m_num1)));}
                            else
                           { _cmrate =( vm_maxrate- f_rate)/vm_maxrate;
                           _cmrate += purchase_wt*(it->m_num1);
                            serverTypeCostSet_today.insert(SFI(f_type,float(_cmrate),it->m_num2,double(it->m_num1)));}

                        }
                }
        
            }
	    }
    }
    int i =0;
    for(auto it_t=serverTypeCostSet_today.begin();it_t!=serverTypeCostSet_today.end();it_t++ )
    {
        if(i ==sct)
        {
            best_server =it_t->m_Type;
            break;
        }
        i+=1;
        
    }
    return best_server;
}

//A*相关函数、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、
void add_vm(int server_id,int vm_id,int vm_node,int vm_core ,int vm_mem)
{
switch(vm_node)
{
    case 0:
        vms_info_map[vm_id]=vector<int>{vm_core,vm_mem,server_id,0};
        servers_info_map[server_id][0] -= vm_core/2;
        servers_info_map[server_id][1] -= vm_core/2;
        servers_info_map[server_id][2] -= vm_mem/2;
        servers_info_map[server_id][3] -= vm_mem/2;
        servers_info_map[server_id][4] += vm_core;
        servers_info_map[server_id][5] += vm_mem;
        server_vminfo_map[server_id].push_back(vector<int>{vm_id,vm_core,vm_mem,0});
        break;
    case 1:
        vms_info_map[vm_id]=vector<int>{vm_core,vm_mem,server_id,1};
        servers_info_map[server_id][0] -= vm_core;
        servers_info_map[server_id][2] -= vm_mem;
        servers_info_map[server_id][4] += vm_core;
        servers_info_map[server_id][5] += vm_mem;
        server_vminfo_map[server_id].push_back(vector<int>{vm_id,vm_core,vm_mem,1});
        break;
    case 2:
        vms_info_map[vm_id]=vector<int>{vm_core,vm_mem,server_id,2};
        servers_info_map[server_id][1] -= vm_core;
        servers_info_map[server_id][3] -= vm_mem;
        servers_info_map[server_id][4] += vm_core;
        servers_info_map[server_id][5] += vm_mem;
        server_vminfo_map[server_id].push_back(vector<int>{vm_id,vm_core,vm_mem,2});
        break;
}
}
int do_opers(vector<IntIntInt> _oper_list,vector<vector< int>>_reqs_list) //确定了最佳oper_list后，在真实世界复现这些操作
{
    int _count = _oper_list.size();
    int _purchase_num =0;
    int _req_size =_reqs_list.size();
    for(int i=1;i<_count;i++)
    {
        int _req_count = i-_purchase_num-1;
        int _server_id = _oper_list[i].m_serverid;
        int _vm_id ;
        int _core_vm ;
        int _mem_vm ;
        switch(_oper_list[i].m_node)
        {
            case -1: //删除
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                del_vm(_server_id,_vm_id,0,_core_vm,_mem_vm);
                break;
            case -2: //删除
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                del_vm(_server_id,_vm_id,1,_core_vm,_mem_vm);
                break;
            case -3: //删除
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                del_vm(_server_id,_vm_id,2,_core_vm,_mem_vm);
                break;
            case 0: //_server_id双节点添加
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                add_vm(_server_id,_vm_id,0,_core_vm,_mem_vm);
                break;
            case 1://_server_idA节点添加
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                add_vm(_server_id,_vm_id,1,_core_vm,_mem_vm);
                break;
            case 2://_server_idB节点添加
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                add_vm(_server_id,_vm_id,2,_core_vm,_mem_vm);
                break;
            case 3://购买
                _purchase_num++;
                int half_core = server_type_map[best_server_type][0]/2;
                int half_mem = server_type_map[best_server_type][1]/2;
                servers_info_map[server_id_count] = vector<int>{half_core,half_core,half_mem,half_mem,0,0 };

		//添加服务器编号类型信息
		serverIdType_map[server_id_count] = best_server_type;

                server_id_count++;
                break;
        }
    }
    return _purchase_num; //返回oper_list中购买操作的数量，用于输出购买请求
}

void cancel_opers(vector<IntIntInt> _oper_list,vector<vector< int>>_reqs_list,int _req_head)   //时间回溯，逆执行opers
{
    int _count = _oper_list.size()-1;
    if(_count>=0)
    {
    int _req_count = _req_head-1;
    int _purchase_num =0;
    for(int i=_count;i>0;i--)
    {
        int _server_id = _oper_list[i].m_serverid;
        int _vm_id ;
        int _core_vm ;
        int _mem_vm ;

        switch(_oper_list[i].m_node)
        {
            case -1:
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                add_vm(_server_id,_vm_id,0,_core_vm,_mem_vm);
                _req_count-=1;
                break;
            case -2:
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                add_vm(_server_id,_vm_id,1,_core_vm,_mem_vm);
                _req_count-=1;
                break;
            case -3:
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                add_vm(_server_id,_vm_id,2,_core_vm,_mem_vm);
                _req_count-=1;
                break;
            case 0:
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                del_vm(_server_id,_vm_id,0,_core_vm,_mem_vm);
                _req_count-=1;
                break;
            case 1:
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                del_vm(_server_id,_vm_id,1,_core_vm,_mem_vm);
                _req_count-=1;
                break;
            case 2:
                _vm_id = _reqs_list[_req_count][4];
                _core_vm = _reqs_list[_req_count][1];
                _mem_vm =  _reqs_list[_req_count][2];
                del_vm(_server_id,_vm_id,2,_core_vm,_mem_vm);
                _req_count-=1;
                break;
            case 3:
                server_id_count-=1;
                auto it = servers_info_map.find(server_id_count);
                servers_info_map.erase(it);
                auto it1 = serverIdType_map.find(server_id_count);
                serverIdType_map.erase(it1);
                break;
        }
    }}
}

void climbmount_setvm(vector<vector<int>> reqs_aday,int sct)
{
    open_list_sct.clear();
    for(int i_sct =0;i_sct<sct;i_sct++)
    {
        
        oper_list_sct=FVIII (0.0,{{i_sct,i_sct,i_sct}});

        float F_sct = 0.0;
        
        oper_list_cb.clear();
        float F = 0.0 ;                 //定义估价函数值 
        int _reqs_size =reqs_aday.size(); 
        purchase_num_climb =0;      //清零今日购买数量
        bool type_set =0;           //0 表示还没有确定今天的购置类型
        for(int i =0;i<_reqs_size;i++) //遍历今天的每一步
        {    //读取请求信息
            open_list_cb.clear();   //清零用于保存这一步所有可能操作的set
            // 读取这一步的所有信息
            int core_vm = reqs_aday[i][1];
            int mem_vm = reqs_aday[i][2];
            int vm_isdouble = reqs_aday[i][3];
            int vm_id = reqs_aday[i][4];
            int oper;
            int open_list_size = 0;
            int cm_err_s;
            int cm_err_v = core_vm - mem_vm;
            // float rate_v=(float)core_vm/mem_vm;
            float rate_s;
            while(open_list_size<max_open_list_size) //限制探索的可能数量  
            {    //首先进行open_list赋初值

                if(reqs_aday[i][0]==0) //如果是删除请求
                {
                    F = 0.0;
                    int _loc_id = vms_info_map[vm_id][2];
                    int _loc_node = vms_info_map[vm_id][3]; 
                    _loc_node = -_loc_node-1;
                    open_list_cb.emplace(F,vm_id,_loc_id,_loc_node);   //把这个操作可能插入到open_list_cb中

                }


                if(reqs_aday[i][0]==1)  //如果是添加请求
                {
                    //计算这个请求的所有可选操作：
                    if(reqs_aday[i][3]==0)    //如果该虚拟机为单节点型
                    {
                        // if(open_list_size>max_open_list_size)
                        // {break;}
                        for(int j=0;j<server_id_count;j++)  //遍历所有服务器id
                        {
                            
                            if(open_list_size>max_open_list_size)
                            {break;}
                            int _server_core_A = servers_info_map[j][0];
                            int _server_core_B = servers_info_map[j][1];
                            int _server_mem_A = servers_info_map[j][2];
                            int _server_mem_B = servers_info_map[j][3];
                            // int _server_core_A = servers_info_map[j][0];
                            

                            if(_server_core_A>=core_vm&&_server_mem_A>=mem_vm) //如果j服务器A节点放得下
                            {
                                if(servers_info_map[j][4]!=0) //如果j服务器不为空服务器
                                {
                                    
                                    F= vl_wt2*value_piancha*(servers_info_map[j][0]+servers_info_map[j][1])+servers_info_map[j][2]+servers_info_map[j][3];
                                    cm_err_s = _server_core_A-_server_mem_A;
                                    if(cm_err_s*cm_err_v<0)
                                    {F+=1000;}
                                    open_list_cb.emplace(F,vm_id,j,1);  //把这个操作可能插入到open_list_cb中
                                    open_list_size++;
                                    // break;
                                }
                                else
                                {
                                    F= vl_wt2*value_piancha*(servers_info_map[j][0]+servers_info_map[j][1])+servers_info_map[j][2]+servers_info_map[j][3]  ;
                                    cm_err_s = _server_core_A-_server_mem_A;
                                    if(cm_err_s*cm_err_v<0)
                                    {F+=1000;}
                                    F +=10000;//占用了空服务器的函数值

                                    open_list_cb.emplace(F,vm_id,j,1);//把这个操作可能插入到open_list_cb中
                                    open_list_size++;
                                }

                            }
                            if(servers_info_map[j][1]>=core_vm&&servers_info_map[j][3]>=mem_vm) //如果j服务器A放不下的话，看B节点放得下不，后续如果启发函数涉及了节点层面，这里可以把else去掉增加可能性
                            {
                                if(servers_info_map[j][4]!=0) //如果j服务器不为空服务器
                                {
                                    F= vl_wt2*value_piancha*(servers_info_map[j][0]+servers_info_map[j][1])+servers_info_map[j][2]+servers_info_map[j][3]  ;
                                    // F =j;  
                                    cm_err_s = _server_core_B-_server_mem_B;
                                    if(cm_err_s*cm_err_v<0)
                                    {F+=1000;}
                                    open_list_cb.emplace(F,vm_id,j,2);  //把这个操作插入到open_list_cb中
                                    open_list_size++;
                                }
                                else
                                {
                                    F= vl_wt2*value_piancha*(servers_info_map[j][0]+servers_info_map[j][1])+servers_info_map[j][2]+servers_info_map[j][3]  ;
                                    F +=10000;
                                    cm_err_s = _server_core_B-_server_mem_B;
                                    if(cm_err_s*cm_err_v<0)
                                    {F+=1000;}
                                    open_list_cb.emplace(F,vm_id,j,2); //把这个操作插入到open_list_cb中
                                    open_list_size++;
                                }
                            }

                        }
                    }
                    if(reqs_aday[i][3]==1) //如果该虚拟机是双节点型
                    {
                        // if(open_list_size>max_open_list_size)
                        // {break;}
                        for(int j=0;j<server_id_count;j++)
                        {
                            int _server_core_A = servers_info_map[j][0];
                            int _server_core_B = servers_info_map[j][1];
                            int _server_mem_A = servers_info_map[j][2];
                            int _server_mem_B = servers_info_map[j][3];
                            if(open_list_size>max_open_list_size)
                            {break;}
                            if(servers_info_map[j][0]>=core_vm/2&&servers_info_map[j][2]>=mem_vm/2)
                            {
                                if(servers_info_map[j][1]>=core_vm/2&&servers_info_map[j][3]>=mem_vm/2)
                                {
                                    if(servers_info_map[j][4]!=0) //如果j服务器不为空服务器
                                    {
                                        
                                        // F =j;  
                                        F= vl_wt2*value_piancha*(servers_info_map[j][0]+servers_info_map[j][1])+servers_info_map[j][2]+servers_info_map[j][3];
                                        cm_err_s = _server_core_A+_server_core_B-_server_mem_B-_server_mem_A;
                                        if(cm_err_s*cm_err_v<0)
                                        {F+=1000;}
                                        open_list_cb.emplace(F,vm_id,j,0); //把这个操作插入到open_list_cb中
                                        open_list_size++;
                                    }
                                    else
                                    {
                                        F= vl_wt2*value_piancha*(servers_info_map[j][0]+servers_info_map[j][1])+servers_info_map[j][2]+servers_info_map[j][3];
                                        F +=10000;
                                        cm_err_s = _server_core_A+_server_core_B-_server_mem_B-_server_mem_A;
                                        if(cm_err_s*cm_err_v<0)
                                        {F+=1000;}
                                        open_list_cb.emplace(F,vm_id,j,0); //把这个操作插入到open_list_cb中
                                        open_list_size++;
                                    }
                                }
                            }
                        }
                    }
                    
                    F = (float)1000000.0;
                    open_list_cb.emplace(F,vm_id,0,3); //购买请求,购买应该比部署进新服务器代价更高
                    open_list_size ++;
                }
                break;
            }
            // oper_list = open_list_cb.begin();
            F = open_list_cb.begin()->m_F;
            int _server_id = open_list_cb.begin()->m_serverid;
            // vm_id = open_list_cb.begin()->m_vmid;
            // int _serverid = open_list_cb.begin()->m_serverid;
            int _oper = open_list_cb.begin()->m_oper;
            // cout<<"选取的 F = "<<F<<endl;
            // cout<<"此时的服务器数量："<<server_id_count<<endl;
            oper_list_cb.emplace_back(F,vm_id,_server_id,_oper) ;

            oper_list_sct.m_OP.emplace_back(vm_id,_server_id,_oper);


            switch(_oper)
            {
                case -1: //删除
                    del_vm(_server_id,vm_id,0,core_vm,mem_vm);
                    break;
                case -2: //删除
                    del_vm(_server_id,vm_id,1,core_vm,mem_vm);
                    break;
                case -3: //删除
                    del_vm(_server_id,vm_id,2,core_vm,mem_vm);
                    break;
                case 0: //_server_id双节点添加
                    add_vm(_server_id,vm_id,0,core_vm,mem_vm);
                    break;
                case 1://_server_idA节点添加
                    add_vm(_server_id,vm_id,1,core_vm,mem_vm);
                    break;
                case 2://_server_idB节点添加
                    add_vm(_server_id,vm_id,2,core_vm,mem_vm);
                    break;
                case 3://购买
                    purchase_num_climb++;
                    if(type_set ==0)
                        {
                            int day_allvm_cores =0;
                            int day_allvm_mem =0;
                            int max_req_cores=0;
                            int max_req_mems=0;
                            for(int ii=i;ii<_reqs_size;ii++)
                            {
                                int core_vm = reqs_aday[ii][1];
                                int mem_vm = reqs_aday[ii][2];
                                int vm_isdouble = reqs_aday[ii][3];
                                int vm_id = reqs_aday[ii][4];
                                day_allvm_cores += core_vm;
                                day_allvm_mem += mem_vm;
                                if(vm_isdouble==1)
                                {
                                    if(max_req_cores<core_vm)
                                    {max_req_cores = core_vm;}
                                    if(max_req_mems<mem_vm)
                                    {max_req_mems = mem_vm;}
                                }
                                if(vm_isdouble==0)
                                {
                                    if(max_req_cores<core_vm*2)
                                    {max_req_cores = core_vm*2;}
                                    if(max_req_mems<mem_vm*2)
                                    {max_req_mems = mem_vm*2;}
                                }
                            }
                        //得到当前性价比最高的可以装下当前请求的服务器
                            best_server_type = Find_server(day_allvm_cores,day_allvm_mem,max_need_core,max_need_mem,i_sct);
                            type_set =1;
                            // outfile<<"天数：剩余未部署核数，内存数   ：   "<<today<<" : "<<day_allvm_cores<<","<<day_allvm_mem<<endl;
                        }
                    // cout<<"今天的最佳服务器为："<<best_server_type<<endl;
                    int half_core = server_type_map[best_server_type][0]/2;
                    int half_mem = server_type_map[best_server_type][1]/2;

                    oper_list_sct.m_F+=(float)server_type_map[best_server_type][2]/(total_day-today)+piancha_run_purchse*server_type_map[best_server_type][3];
                    servers_info_map[server_id_count] = vector<int>{half_core,half_core,half_mem,half_mem,0,0 };
                    i-=1;
                    //添加服务器编号类型信息
                    serverIdType_map[server_id_count] = best_server_type;

                    server_id_count++;
                    break;
            }


        
        }
        open_list_sct.insert(oper_list_sct);
        if(server_choose_times !=1)
        {cancel_opers(oper_list_sct.m_OP,reqs_aday,reqs_aday.size());}
        
    }
    if(server_choose_times!=1)
    {oper_list_sct = *open_list_sct.begin();
    purchase_num_climb =do_opers(oper_list_sct.m_OP,reqs_aday);}
    int choose_type_num =0;
    for(auto it_t =serverTypeCostSet_today.begin();it_t!=serverTypeCostSet_today.end();it_t++)
    {
        if(choose_type_num == oper_list_sct.m_OP.front().m_serverid)
        {
            best_server_type = it_t->m_Type;
            break;
        }
        choose_type_num++;
    }

    // purchase_num = do_opers(oper_list_copy,reqs_aday);
    // cout<<"今天的天数："<<today<<endl;
    if(purchase_num_climb ==0)
    {
        cout<<"(purchase, 0)"<<endl;
    // outfile<<"(purchase, 0)"<<endl;
    }
    else
    {
        cout<<"(purchase, 1)"<<endl;
        // outfile<<"(purchase, 1)"<<endl;
        string _purchase_body ="(, )";
        _purchase_body.insert(3,int2str(purchase_num_climb));
        _purchase_body.insert(1,best_server_type);
        cout<<_purchase_body<<endl;
        // outfile<<_purchase_body<<endl;

    }
    // 输出迁移指令
    string mig_head = "(migration, )";
    mig_head.insert(12,int2str(mig_list.size()));
    cout<<mig_head<<endl;
    // outfile<<mig_head<<endl;
    if(mig_list.size()!=0)
    {
        for(int ij =0; ij<mig_list.size();ij++)
        {
            if(mig_list[ij].m_node==0)
            {
                cout<<"("<<mig_list[ij].m_vmid<<", "<<mig_list[ij].m_serverid<<")"<<endl;
            // outfile<<"("<<mig_list[ij].m_vmid<<", "<<mig_list[ij].m_serverid<<")"<<endl;
            }
            else if(mig_list[ij].m_node==1)
            {
                cout<<"("<<mig_list[ij].m_vmid<<", "<<mig_list[ij].m_serverid<<", A)"<<endl;
            // outfile<<"("<<mig_list[ij].m_vmid<<", "<<mig_list[ij].m_serverid<<", A)"<<endl;
            }
            else if(mig_list[ij].m_node==2)
            {
                cout<<"("<<mig_list[ij].m_vmid<<", "<<mig_list[ij].m_serverid<<", B)"<<endl;
            // outfile<<"("<<mig_list[ij].m_vmid<<", "<<mig_list[ij].m_serverid<<", B)"<<endl;
            }
        }
    }
    // 输出部署指令
    // cout<<"今日待处理请求数："<<set_vm_map[today].size()<<endl;
    // for(auto it = set_vm_map[today].begin();it!=set_vm_map[today].end();it++)
    int _oper_size = oper_list_sct.m_OP.size();
    
    for(int i=1;i<_oper_size;i++)
    {
        
        int _id = oper_list_sct.m_OP[i].m_serverid ;//////
        // _id = 0;

        int oper = oper_list_sct.m_OP[i].m_node;
        // if(_id>(server_id_count-1))
        // {cout<<"出现服务器id溢出了！!!!!!!!!!!!!!!"<<endl;}
        // string out = "()";
        switch(oper)
        {
            case -1:break;
            case -2:break;
            case -3:break;
            case 0 :
            cout<< "(" <<_id<< ")"<<endl;
                    // outfile<< "(" <<_id<< ")"<<endl;
                    break;
            case 1 :
            cout<< "(" <<_id<< ", " <<"A"<< ")"<<endl;
                    // outfile<< "(" <<_id<< ", " <<"A"<< ")"<<endl;
                    break;
            case 2 :
            cout<< "(" <<_id<< ", " <<"B"<< ")"<<endl;
                    // outfile<< "(" <<_id<< ", " <<"B"<< ")"<<endl;
                    break;
            case 3 :break;
        }
    }
}

void read_req_map()
{
    
    vector<vector<int>> reqs_aday ;
    
    // total_day = reqs_map.size();
    
    for(today = 0;today<total_day;today++)
    {
        mig_list.clear();

        migration();

        vector<string> purchase_list;
        reqs_aday.clear();
        reqs_aday = reqs_map[today];

        if(1)
        {   
            // for(int sct =0;sct<server_choose_times;sct++)
            // {
                // oper_list_sct=FVIII (0.0,{{sct,sct,sct}});
                climbmount_setvm(reqs_aday,server_choose_times);
            // }
        }


        //更新map
        if(today<total_day - first_read_lenth)
        {
            string s;
            string type_temp; 
            string id_temp;
            cin>>s;
            int reqs_num = str2int(s);
            for(int j =0;j<reqs_num;j++)
            {
                int core_vm;
                int mem_vm;
                int is_double;
                int need_core;
                int need_mem;
                cin>>s;
                if(s[1]=='a')
                {
                    cin>>type_temp>>id_temp;
                    type_temp=type_temp.substr(0,type_temp.size()-1);
                    id_temp=id_temp.substr(0,id_temp.size()-1);
                    core_vm = vm_type_map[type_temp][0];
                    mem_vm = vm_type_map[type_temp][1];
                    all_vmcores += core_vm;
                    all_vmmem += mem_vm;
                    if(all_vmcores>max_vmcores){max_vmcores=all_vmcores;};
                    if(all_vmmem>max_vmmem){max_vmmem=all_vmmem;};
                    is_double = vm_type_map[type_temp][2];
                    if(is_double)
                    {   need_core = core_vm/2;
                        need_mem = mem_vm/2;
                        if(max_need_core<need_core)
                        {max_need_core=need_core;
                        if(need_mem>max_core_mem)
                        {max_core_mem =need_mem; }
                        }
                        if(max_need_mem<need_mem)
                        {max_need_mem=need_mem;
                        if(need_core>max_mem_core)
                        {max_mem_core =need_core; }}
                    }
                    if(!is_double)
                    {
                        need_core = core_vm*2;
                        need_mem = mem_vm*2;
                        if(max_need_core<need_core)
                        {max_need_core=need_core;
                        if(need_mem>max_core_mem)
                        {max_core_mem =need_mem; }
                        }
                        if(max_need_mem<need_mem)
                        {max_need_mem=need_mem;
                        if(need_core>max_mem_core)
                        {max_mem_core =need_core; }}
                    }
                    vmIdType_map[id_temp]=type_temp;
                    //保存请求
                    reqs_map[today+first_read_lenth].push_back({1,core_vm,mem_vm,is_double,str2int(id_temp)});


                }
                else if(s[1]=='d')
                {
                    cin>>id_temp;
                    id_temp=id_temp.substr(0,id_temp.size()-1);
                    type_temp = vmIdType_map[id_temp];
                    core_vm = vm_type_map[type_temp][0];
                    mem_vm = vm_type_map[type_temp][1];
                    is_double = vm_type_map[type_temp][2];
                    all_vmcores -= core_vm;
                    all_vmmem -= mem_vm;
                    //保存删除请求

                    reqs_map[today+first_read_lenth].push_back({0,core_vm,mem_vm,is_double,str2int(id_temp)});
                }
            }
            value_piancha = (float)(vl_wt*max_vmmem/max_vmcores);
        }

    }
}

int main()
{
    int reqs_count = 0;
    int day = 0;
    int num = 0;
    int amount = 0;
    string s;

    start = clock();
    // 读取服务器和虚拟机类型数据，建立服务器类型表和虚拟机类型表--------------------------------------------------------------------------------------

    //std::freopen("D:\\training-1.txt","rb",stdin);
    // std::freopen("training-1.txt","rb",stdin);
    // std::freopen("trainig-1.txt","rb",stdin);


    cin>>s;
    amount = str2int(s);
    for (int i = 0; i < amount; i++)
    {
        string type_temp,core_temp,mem_temp,purchasecost_temp,runcost_temp;
        cin>>type_temp>>core_temp>>mem_temp>>purchasecost_temp>>runcost_temp;
        generateServer(type_temp,core_temp,mem_temp,purchasecost_temp,runcost_temp);
    }
    // cout<<"服务器类型总数 ： "<<server_type_map.size()<<endl;
    //建立虚拟机类型表
    cin>>s;
    amount = str2int(s);
    for (int i = 0; i < amount; i++)
    {
        string type_temp;
        string core_temp,mem_temp,isdouble;
        cin>>type_temp>>core_temp>>mem_temp>>isdouble;
        generateVm(type_temp,core_temp,mem_temp,isdouble);
    }

    int reqs_num = 0;
    cin>>s;
    total_day = str2int(s);
    cin>>s;
    first_read_lenth = str2int(s);

    //读取第一批请求数据，计算内存和核数出现的最大值

    int need_mem =0;
    int need_core =0;
    for (int i =0;i<first_read_lenth;i++)
    {
        string type_temp; 
        string id_temp;
        cin>>s;
        reqs_num = str2int(s);
        for(int j =0;j<reqs_num;j++)
        {
            int core_vm;
            int mem_vm;
            int is_double;
            cin>>s;
            if(s[1]=='a')
            {
                cin>>type_temp>>id_temp;
                type_temp=type_temp.substr(0,type_temp.size()-1);
                id_temp=id_temp.substr(0,id_temp.size()-1);
                core_vm = vm_type_map[type_temp][0];
                mem_vm = vm_type_map[type_temp][1];
                all_vmcores += core_vm;
                all_vmmem += mem_vm;
                if(all_vmcores>max_vmcores){max_vmcores=all_vmcores;};
                if(all_vmmem>max_vmmem){max_vmmem=all_vmmem;};
                is_double = vm_type_map[type_temp][2];
                if(is_double)
                {   need_core = core_vm/2;
                    need_mem = mem_vm/2;
                    if(max_need_core<need_core)
                    {max_need_core=need_core;
                    if(need_mem>max_core_mem)
                    {max_core_mem =need_mem; }
                    }
                    if(max_need_mem<need_mem)
                    {max_need_mem=need_mem;
                    if(need_core>max_mem_core)
                    {max_mem_core =need_core; }}
                }
                if(!is_double)
                {
                    need_core = core_vm*2;
                    need_mem = mem_vm*2;
                    if(max_need_core<need_core)
                    {max_need_core=need_core;
                    if(need_mem>max_core_mem)
                    {max_core_mem =need_mem; }
                    }
                    if(max_need_mem<need_mem)
                    {max_need_mem=need_mem;
                    if(need_core>max_mem_core)
                    {max_mem_core =need_core; }}
                }
                vmIdType_map[id_temp]=type_temp;
                //保存请求
                reqs_map[i].push_back({1,core_vm,mem_vm,is_double,str2int(id_temp)});


            }
            else if(s[1]=='d')
            {
                cin>>id_temp;
                id_temp=id_temp.substr(0,id_temp.size()-1);
                type_temp = vmIdType_map[id_temp];
                core_vm = vm_type_map[type_temp][0];
                mem_vm = vm_type_map[type_temp][1];
                is_double = vm_type_map[type_temp][2];
                all_vmcores -= core_vm;
                all_vmmem -= mem_vm;
                //保存删除请求

                reqs_map[i].push_back({0,core_vm,mem_vm,is_double,str2int(id_temp)});
            }
        }
    }
    
    int err_cm =max_vmcores-max_vmmem;

    //计算各服务器相对性价比，并建立服务器类型-性价比排序表（相对性价比，服务器类型名）
    value_piancha = (float)(vl_wt*max_vmmem/max_vmcores);
    for(auto it =server_type_map.begin();it!=server_type_map.end();it++)
    {
        float cost_perday = 0.0;
        string type_temp = it->first;
        int servercost_temp = it->second[2];
        int runcost_temp = it->second[3];
        int server_core_temp =  it->second[0];
        int server_mem_temp = it->second[1];
        int core_mem_err = server_core_temp - server_mem_temp;
	    double server_rate = double(server_core_temp)/double(server_mem_temp);
        cost_perday = (float)(servercost_temp/total_day + runcost_temp*piancha_run_purchse)/(server_core_temp*value_piancha+server_mem_temp);
        serverTypeCostSet.insert(SFI(type_temp,cost_perday,core_mem_err,server_rate));
    }
    read_req_map();
    //fclose(stdin);
    //outfile.close();
    return 0 ;
}
