netsh advfirewall firewall delete rule name="Open Port 18889"
netsh advfirewall firewall add rule name="Open Port 18889" dir=in action=allow protocol=TCP localport=18889
