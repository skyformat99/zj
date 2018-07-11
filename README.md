# zj
jump server written in C and raft.

## 亮点
- 业务流程：        
	- 以DevGroup(开发组)为基本单位进行授权管理，免去了繁杂的单台申请流程；
	- 使用投票制管理DevGroup成员变动，即：加入或删除成员时，只需要半数以上成员投票通过，而不必每次通过超管审批授权，实现了DevGroup团队内部的“灵活自管理”；

- 可用性：        
	- 中枢系统采用基于raft算法开发的分布式架构，避免了单点故障，提供高可用服务；

- 性能：        
	- 分布式多节点，可承载更多的客户端连接，提供更强、更流畅的服务能力；
	- zj服务端采用单进程直接跳转模式，较之现存的跳板机，每个连接节省一个进程的开销；
	- 已获授权的DevGroup及主机信息，缓存于客户端本地，而非每次无谓的访问服务端，客户端体验得到提升；

- 功能：        
	- zj服务端采用的单进程直接跳转模式，是原子性的，由此杜绝了过往常见的多进程不协调导致的花屏、终端尺寸失灵、僵死进程残留等影响体验及性能的问题；
	- 客户端与最终服务器之间，可进行方便的双向文件传输；
	- 支持高效的批量管理功能（类似于ansible功能）；

- 安全性：        
	- 传统的跳板机必须允许用户在服务端停留，以选择要登陆的主机，且此服务器通常拥有最高权限连通大量的最终服务器，由此埋下了巨大的安全隐患，一旦此中心服务器被攻破，即意味着大量最终服务器的完全沦陷；
	- zj服务端采用的单进程直接跳转模式，提供了更强的安全保障，因为提供跳转连接功能的节点，本身无须拥有连接任何最终服务器的权限，而且由于客户端是在本地选择待连接的目标机，因此跳转节点完全不需要提供客户端中途停留的机会；

- 开发稳健性：        
	- 测试代码量 > N * 正文代码量；
	- 极少的依赖，极简的代码(Less code, More security!)。

## TODO
- 完善审计功能

## 前端模块示例
- ![](前端模块示例.png)
- ![](https://raw.githubusercontent.com/kt10/zj/master/demo/%E5%89%8D%E7%AB%AF%E6%A8%A1%E5%9D%97%E7%A4%BA%E4%BE%8B.png)
