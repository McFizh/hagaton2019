#!/usr/bin/env bash

# disable selinux for current boot
setenforce 0

# disable selinux permanently
sed -i 's/SELINUX=enforcing/SELINUX=disabled/' /etc/sysconfig/selinux
sed -i 's/SELINUX=enforcing/SELINUX=disabled/' /etc/selinux/config

rpm --import https://github.com/rabbitmq/signing-keys/releases/download/2.0/rabbitmq-release-signing-key.asc
cp /vagrant/rabbitmq.repo /etc/yum.repos.d/

yum install -q -y epel-release
yum install -q -y rabbitmq-server avahi avahi-tools
yum update -q -y

cp /vagrant/rabbitmq_*.service /etc/avahi/services/

systemctl enable rabbitmq-server
systemctl enable avahi-daemon

systemctl start rabbitmq-server
systemctl start avahi-daemon

rabbitmq-plugins enable rabbitmq_mqtt
rabbitmq-plugins enable rabbitmq_web_mqtt

rabbitmqctl add_user test test
rabbitmqctl set_permissions -p / test ".*" ".*" ".*"
