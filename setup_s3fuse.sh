apt upgrade
apt-get install fuse g++ libfuse-dev libcurl4-openssl-dev libxml2-dev mime-support automake libtool
apt install s3fs
mkdir /home/ubuntu/s3fuse
s3fs metamorphic-rocks /home/ubuntu/s3fuse
