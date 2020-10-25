thrift-0.12.0 --gen cpp:cob_style saasame.thrift
thrift-0.12.0 --gen php:server -r saasame.thrift
thrift-0.12.0 -r --gen py saasame.thrift
cscript fix_php_thrift.vbs .\gen-php\saasame\transport
pause
