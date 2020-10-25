netsh advfirewall firewall delete rule name="Open Port 80"
netsh advfirewall firewall delete rule name="Open Port 443"
netsh advfirewall firewall delete rule name="Open Port 18888"
netsh advfirewall firewall delete rule name="Open Port 18890"
netsh advfirewall firewall delete rule name="Open Port 18891"
netsh advfirewall firewall delete rule name="Open Port 18892"
netsh advfirewall firewall delete rule name="Open Port 18893"
netsh advfirewall firewall delete rule name="Open Port 28891"
netsh advfirewall firewall delete rule name="Open Port 18443"

netsh advfirewall firewall add rule name="Open Port 80" dir=in action=allow protocol=TCP localport=80
netsh advfirewall firewall add rule name="Open Port 443" dir=in action=allow protocol=TCP localport=443
netsh advfirewall firewall add rule name="Open Port 18888" dir=in action=allow protocol=TCP localport=18888
netsh advfirewall firewall add rule name="Open Port 18890" dir=in action=allow protocol=TCP localport=18890
netsh advfirewall firewall add rule name="Open Port 18891" dir=in action=allow protocol=TCP localport=18891
netsh advfirewall firewall add rule name="Open Port 18892" dir=in action=allow protocol=TCP localport=18892
netsh advfirewall firewall add rule name="Open Port 18893" dir=in action=allow protocol=TCP localport=18893
netsh advfirewall firewall add rule name="Open Port 28891" dir=in action=allow protocol=TCP localport=28891
netsh advfirewall firewall add rule name="Open Port 18443" dir=in action=allow protocol=TCP localport=18443