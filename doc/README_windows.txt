SLIMCoin 0.3.0 BETA

Copyright (c) 2013-2014 SLIMCoin Developers
Copyright (c) 2011-2013 PPCoin Developers
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Intro
-----
SLIMCoin is a free open source project derived from PPCoin and Bitcoin, with
the goal of providing a long-term energy-efficient crypto-currency.
Built on the foundation of Bitcoin, innovations such as proof-of-burn
help further advance the field of crypto-currency.


Setup
-----
After completing windows setup then run heat-qt.
Alternatively you can run windows command line (cmd) in heat program dir.
  cd daemon
  heatd
You would need to create a configuration file heat.conf in the default
wallet directory. Grant access to heatd/heat-qt in anti-virus and firewall
applications if necessary.

The software automatically finds other nodes to connect to.  You can
enable Universal Plug and Play (UPnP) with your router/firewall
or forward port 9901 (TCP) to your computer so you can receive
incoming connections.  SLIMCoin works without incoming connections,
but allowing incoming connections helps the SLIMCoin network.


Upgrade
-------
All your existing coins/transactions should be intact with the upgrade.
To upgrade from 0.2, first backup wallet
heatd backupwallet <destination_backup_file>
Then shutdown heatd by
heatd stop
Remove files inside wallet directory other than wallet.dat and heat.conf
Start up the new heat-qt (0.3.0). It would start re-download of block chain.


See the documentation/wiki at the heat website:
  http://www.heat.org/
for help and more information.


------------------
Bitcoin 0.6.3 BETA

Copyright (c) 2009-2012 Bitcoin Developers
Distributed under the MIT/X11 software license, see the accompanying
file license.txt or http://www.opensource.org/licenses/mit-license.php.
This product includes software developed by the OpenSSL Project for use in
the OpenSSL Toolkit (http://www.openssl.org/).  This product includes
cryptographic software written by Eric Young (eay@cryptsoft.com).


Intro
-----
Bitcoin is a free open source peer-to-peer electronic cash system that is
completely decentralized, without the need for a central server or trusted
parties.  Users hold the crypto keys to their own money and transact directly
with each other, with the help of a P2P network to check for double-spending.


See the bitcoin wiki at:
  https://en.bitcoin.it/wiki/Main_Page
for more help and information.
