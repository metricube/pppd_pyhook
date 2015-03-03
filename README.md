# pppd_pyhook

A plugin for pppd that allows for pppd hooks and notifications to be implemented in python.  Currently it defines hooks sufficient to use an alternative CHAP secret provider.

## Installation

    # Ensure the dependencies are installed
    sudo yum install ppp ppp-devel gcc python-devel make

    # Clone this repository
    git clone https://github.com/metricube/pppd_pyhook.git

    # Make and install
    cd pppd_pyhook
    make
    sudo make install

 - This will install pyhook.so in to /usr/lib64/pppd/_ppp-version_
 - Install hooks.py in to /etc/ppp/hooks.py

 - To load the module add the following line to /etc/ppp/options.xxx file

        plugin pyhook.so

## Usage

Edit /etc/ppp/hooks.py as required

 - If you don't want a particular hook defined - rename/remove it from hooks.py

## Troubleshooting & Limitations

 - Tested on CentOS6 and Ubuntu x86_64 only
 - Spurious errors.  Check if SELinux is enabled.  It limits greatly what pppd (and hence the python hooks) can do.  Check the SELinux audit log (/var/log/audit/audit.log)
