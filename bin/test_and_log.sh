#!/bin/bash

./nsn -ltestout/IF_DEL -Ltestout/IF_ADD -atestout/ADDR_DEL -Atestout/ADDR_ADD -rtestout/RT_DEL -Rtestout/RT_ADD -V | tee testout/nsnlog.log

