prefix = /usr/local

INSTALL=install

all:

#
# reference: https://github.com/Old-Man-Programmer/tree/blob/master/Makefile
#
install:
	$(INSTALL) -D ./nesfr_vr_ros2_docker.sh $(DESTDIR)$(prefix)/bin/nesfr_vr_ros2_docker.sh
	$(INSTALL) -D -m 664  ./nesfr_vr_ros2_docker.service $(DESTDIR)$(prefix)/share/nesfr_vr_ros2_docker/nesfr_vr_ros2_docker.service

clean:

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/nesfr_vr_ros2_docker.sh
	-rm -f $(DESTDIR)$(prefix)/share/nesfr_vr_ros2_docker/nesfr_vr_ros2_docker.service

.PHONY: all install clean distclean uninstall
