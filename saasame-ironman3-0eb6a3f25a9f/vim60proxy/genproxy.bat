if not exist .\vim25.h (
    call ..\opensources\gsoap_2.8.44\gsoap\bin\win32\wsdl2h.exe .\wsdl\vim25\vimService.wsdl -o vim25.h -nVim25Api -NVim25Service -w -g -b -R -x
    call ..\opensources\gsoap_2.8.44\gsoap\bin\win32\soapcpp2.exe -CLiwxj vim25.h -I..\opensources\gsoap_2.8.44\gsoap\import
)