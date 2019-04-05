all:
	for osc in */Makefile ; do cd `dirname $$osc` && make && cd .. ; done

clean:
	for osc in */Makefile ; do cd `dirname $$osc` && make clean && cd .. ; done

clear:
	for osc in */Makefile ; do cd `dirname $$osc` && make clear && cd .. ; done

install: all
	for osc in */Makefile ; do cd `dirname $$osc` && make install && cd .. ; done

package: all
	# Put builds in prlgunit director so people can easily download them from github
	-cp */*.prlgunit prlgunit/
	ls -l prlgunit
	
