# 捕获必要的信号，防止用户进入跳板机 SHELL
trap SIGINT SIGTERM SIGPIPE

ConfFile=${HOME}/rjms.conf

# 用户选择界面
select input in `cat ${ConfFile}`
do
	
done
