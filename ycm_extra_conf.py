def FlagsForFile( filename, **kwargs ):
   return {
     'flags': [ '-x', 'c', '-Wall', '-Wextra', '-Werror', '-std=c11', '-isystem', '/usr/local/include', '-isystem', '/usr/include',
         '-I/home/_/zj/deps/nng-1.0.0/__z__/include',
         '-I/home/_/zj/deps/libgit2-0.27.2/__z__/include',
         '-I/home/_/zj/deps/libssh-20180628/__z__/include',
         '-I/home/_/zj/src',
         '-I/home/_/zj/src/MQ',
         '-I/home/_/zj/src/ssh',
         '-I/home/_/zj/src/http',
         '-I/home/_/zj/src/raft',
         '-I/home/_/zj/src/utils',
         '-I/home/_/zj/src/threadpool',
         '-I/home/_/zj/src/sha1',
         '-I/home/_/zj/src/json',
         ],
   }
