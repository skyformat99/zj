def FlagsForFile( filename, **kwargs ):
   return {
     'flags': [ '-x', 'c', '-Wall', '-Wextra', '-Werror', '-std=c11', '-isystem', '/usr/local/include', '-isystem', '/usr/include',
         '-I/home/fh/zj/deps/nng-1.0.0/__z__/include',
         '-I/home/fh/zj/deps/libgit2-0.27.2/__z__/include',
         '-I/home/fh/zj/deps/libssh-20180628/__z__/include',
         '-I/home/fh/zj/src',
         '-I/home/fh/zj/src/MQ',
         '-I/home/fh/zj/src/ssh',
         '-I/home/fh/zj/src/http',
         '-I/home/fh/zj/src/raft',
         '-I/home/fh/zj/src/utils',
         '-I/home/fh/zj/src/threadpool',
         '-I/home/fh/zj/src/sha1',
         '-I/home/fh/zj/src/json',
         ],
   }
