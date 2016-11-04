#!/bin/bash

max_proc_id=31
max_mc_idx=19
max_retry=0
ME_COLD_RESET_DELAY=5;

ME_UTIL="/usr/local/bin/me-util"
ME_UTIL_LOG="/tmp/me-util.log"

#echo "crash dump for Broadwell-DE, version 0.01.01, 2015/07/29"
#echo "script name is   ==> $0"
#echo "whole parameter  ==> $#"
#echo "parameter 1      ==> $1"
#echo "parameter 2      ==> $2"
#echo "parameter 3      ==> $3"

if [ $1 = "time" ]; then
  now=$(date)
  echo "Crash Dump generated at $now"
  exit 0
fi

echo ""

if [ "$#" -ne 3 ]; then

  echo "$0 <slot#> 48 coreid  ==> for CPU 1 CoreID"
	echo "$0 <slot#> 48 tor     ==> for CPU 1 TOR"
	echo "$0 <slot#> 48 msr     ==> for CPU 1 MSR"

	exit 1

elif [ "$2" -ne 48 ]; then

	echo "$0  <slot#> 48 coreid  ==> for CPU 1 CoreID"
	echo "$0  <slot#> 48 tor     ==> for CPU 1 TOR"
	echo "$0  <slot#> 48 msr     ==> for CPU 1 MSR"

	exit 1

elif [ "$3" = "coreid" ]; then

  echo ""
  echo "CPU COREID DUMP:"
  echo "================"
  echo ""

	#PECI RdPkgConfig() "CPUID Read"
	echo "< CPUID Read >"
  cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x05 0xa1 0x00 0x00 0x00 0x00)
  head -n 1 $ME_UTIL_LOG

	#PECI RdPkgConfig() "CPU Microcode Update Revision Read"
	echo "< CPU Microcode Update Revision Read >"
  cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x05 0xa1 0x00 0x00 0x04 0x00)
  head -n 1 $ME_UTIL_LOG

	#PECI RdPkgConfig() "MCA ERROR SOURCE LOG Read"
	echo "< MCA ERROR SOURCE LOG Read -- The socket which MCA_ERR_SRC_LOG[30]=0 is the socket that asserted IERR first >"
  cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x05 0xa1 0x00 0x00 0x05 0x00)
  head -n 1 $ME_UTIL_LOG

	#PECI RdPkgConfig() "Core ID IERR"
	#echo "< Core ID IERR -- determine whether a core in the failing socket asserted an IERR, completion data[3]=1 if a core caused the IERR, data[2:0] is the core ID, and save the value for matching purpose >"
	#ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x05 0xa1 0x00 0x27 0x08 0x08

	if [ "$2" -eq 48 ]; then

		printf "********************************************************\n"
		printf "*                   IERRLOGGINGREG                     *\n"
		printf "********************************************************\n"

    cmdout=$($ME_UTIL $1 0xB8 0x44 0x57 0x01 0x00 0x40 0xA4 0x50 0x18 0x00 0x03)
    head -n 1 $ME_UTIL_LOG

		printf "********************************************************\n"
		printf "*                   MCERRLOGGINGREG                    *\n"
		printf "********************************************************\n"

    cmdout=$($ME_UTIL $1 0xB8 0x44 0x57 0x01 0x00 0x40 0xA8 0x50 0x18 0x00 0x03)
    head -n 1 $ME_UTIL_LOG

	fi

	exit 1

#elif [ "$3" = "tor" ]; then
#	CboIndex=0
#	TorArrayIndex=0
#	param=0	
#	param0=0
#	param1=0
#	BankNumber=0
#	wfc=186
#	rfc=72
#
#	while [ "$CboIndex" -le 13 ]
#	do
#		TorArrayIndex=0
#		while [ "$TorArrayIndex" -le 19 ]
#		do
#			BankNumber=0
#			while [ "$BankNumber" -le 2 ]
#			do
#				param=$(($BankNumber + $(($TorArrayIndex << 2)) + $(($CboIndex << 7))))
#				param0=$(($param & 255))
#				param1=$(($param >> 8))
#				#echo "< CboIndex=$CboIndex, TorArrayIndex=$TorArrayIndex, BankNumber=$BankNumber Read >"
#				#echo "$BankNumber $TorArrayIndex $CboIndex $2 05 05 a1 00 27 $param"
#				printf "%.2x %.2x %.2x %.2x 05 05 a1 00 27 %.4x %.2x" $BankNumber $TorArrayIndex $CboIndex $2 $param $wfc
#				ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x05 0xa1 0x00 0x27 $param0 $param1 
#				#printf " %.2x\n" $rfc
#
#				BankNumber=$(($BankNumber+1))
#			done
#
#			TorArrayIndex=$(($TorArrayIndex+1))
#		done
#
#		CboIndex=$(($CboIndex+1))
#	done
#
#	exit 1

elif [ "$3" = "msr" ]; then

	MC_index=0
	MSR_offset=0
	IA32_MC_CTL_base=0x0400
	IA32_MC_CTL2_base=0x0280
	IA32_MC_STATUS_base=0x0401
	IA32_MC_ADDR_base=0x0402
	IA32_MC_MISC_base=0x0403
	IA32_MCG_CAP=0x0179
	IA32_MCG_STATUS=0x017A
	IA32_MCG_CONTAIN=0x0178

	ProcessorID=0
	retry=0
	return=0
	response=0
	comp=1

  echo ""
  echo "MSR DUMP:"
  echo "========="
  echo ""

	while [ "$MC_index" -le $max_mc_idx ]
	do

		printf "********************************************************\n"
		printf "*                    MC index %02u                       *\n" $MC_index
		printf "********************************************************\n"

		printf "   <<< IA32_MC%u_CTL, ProcessorID from 0 to %u >>> \n" $MC_index $max_proc_id
 
		ProcessorID=0
		retry=0
		while [ "$ProcessorID" -le $max_proc_id ]
		do
			param=$(($IA32_MC_CTL_base + $MSR_offset))
			param0=$(($param & 255))
			param1=$(($param >> 8))
			#echo "< MC_index=$MC_index, MSR_offset=$MSR_offset ProcessorID=$ProcessorID Read >"
			#printf "[issue] %.2x %.4x %.2x %.2x %.2x %.2x %s %s\n" $MC_index $MSR_offset $ProcessorID $param $param0 $param1 $user $passwd
			#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MC%u_CTL MSR_offset=0x%.4x\n" $ProcessorID $param $MC_index $MSR_offset
			#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
			cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
      response=$(head -n 1 $ME_UTIL_LOG)
			comp=$?
			return=$(echo $response | cut -d ' ' -f 4)

			if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
				#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
				retry=$(($retry+1))
				sleep 1
			else
				echo "$response"
				ProcessorID=$(($ProcessorID+1))
				retry=0
			fi
		done



		printf "   <<< IA32_MC%u_CTL2, ProcessorID from 0 to %u >>> \n" $MC_index $max_proc_id

		ProcessorID=0
		retry=0
		while [ "$ProcessorID" -le $max_proc_id ]
		do
			param=$(($IA32_MC_CTL2_base + $MC_index))
			param0=$(($param & 255))
			param1=$(($param >> 8))
			#echo "< MC_index=$MC_index, ProcessorID=$ProcessorID Read >"
			#printf "[issue] %.2x %.2x %.2x %.2x %.2x %s %s\n" $MC_index $ProcessorID $param $param0 $param1 $user $passwd
			#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MC%u_CTL2 MC_index=0x%.4x\n" $ProcessorID $param $MC_index $MC_index
			#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
			cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
      response=$(head -n 1 $ME_UTIL_LOG)
			comp=$?
			return=$(echo $response | cut -d ' ' -f 4)

			if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
				#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
				retry=$(($retry+1))
				sleep 1
			else
				echo "$response"
				ProcessorID=$(($ProcessorID+1))
				retry=0
			fi
		done



		printf "   <<< IA32_MC%u_STATUS, ProcessorID from 0 to %u >>> \n" $MC_index $max_proc_id

		ProcessorID=0
		retry=0
		while [ "$ProcessorID" -le $max_proc_id ]
		do
			param=$(($IA32_MC_STATUS_base + $MSR_offset))
			param0=$(($param & 255))
			param1=$(($param >> 8))
			#echo "< MC_index=$MC_index, MSR_offset=$MSR_offset ProcessorID=$ProcessorID Read >"
			#printf "[issue] %.2x %.4x %.2x %.2x %.2x %.2x %s %s\n" $MC_index $MSR_offset $ProcessorID $param $param0 $param1 $user $passwd
			#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MC%u_STATUS MSR_offset=0x%.4x\n" $ProcessorID $param $MC_index $MSR_offset
			#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
			cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
      response=$(head -n 1 $ME_UTIL_LOG)
			comp=$?
			return=$(echo $response | cut -d ' ' -f 4)

			if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
				#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
				retry=$(($retry+1))
				sleep 1
			else
				echo "$response"
				ProcessorID=$(($ProcessorID+1))
				retry=0
			fi
		done



		printf "   <<< IA32_MC%u_ADDR, ProcessorID from 0 to %u >>> \n" $MC_index $max_proc_id

		ProcessorID=0
		retry=0
		while [ "$ProcessorID" -le $max_proc_id ]
		do
			param=$(($IA32_MC_ADDR_base + $MSR_offset))
			param0=$(($param & 255))
			param1=$(($param >> 8))
			#echo "< MC_index=$MC_index, MSR_offset=$MSR_offset ProcessorID=$ProcessorID Read >"
			#printf "[issue] %.2x %.4x %.2x %.2x %.2x %.2x %s %s\n" $MC_index $MSR_offset $ProcessorID $param $param0 $param1 $user $passwd
			#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MC%u_ADDR MSR_offset=0x%.4x\n" $ProcessorID $param $MC_index $MSR_offset
			#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
			cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
      response=$(head -n 1 $ME_UTIL_LOG)
			comp=$?
			return=$(echo $response | cut -d ' ' -f 4)

			if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
				#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
				retry=$(($retry+1))
				sleep 1
			else
				echo "$response"
				ProcessorID=$(($ProcessorID+1))
				retry=0
			fi
		done



		printf "   <<< IA32_MC%u_MISC, ProcessorID from 0 to %u >>> \n" $MC_index $max_proc_id

		ProcessorID=0
		retry=0
		while [ "$ProcessorID" -le $max_proc_id ] 
		do
			param=$(($IA32_MC_MISC_base + $MSR_offset))
			param0=$(($param & 255))
			param1=$(($param >> 8))
			#echo "< MC_index=$MC_index, MSR_offset=$MSR_offset ProcessorID=$ProcessorID Read >"
			#printf "[issue] %.2x %.4x %.2x %.2x %.2x %.2x %s %s\n" $MC_index $MSR_offset $ProcessorID $param $param0 $param1 $user $passwd
			#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MC%u_MISC MSR_offset=0x%.4x\n" $ProcessorID $param $MC_index $MSR_offset
			#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
			cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
      response=$(head -n 1 $ME_UTIL_LOG)
			comp=$?
			return=$(echo $response | cut -d ' ' -f 4)

			if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
				#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
				retry=$(($retry+1))
				sleep 1
			else
				echo "$response"
				ProcessorID=$(($ProcessorID+1))
				retry=0
			fi
		done


		MC_index=$(($MC_index+1))
		MSR_offset=$(($MSR_offset+4))


#		printf "********************************************************\n"
#		printf "*             ME Cold Reset, and wait for %u sec        *\n" $ME_COLD_RESET_DELAY
#		printf "********************************************************\n"

#		ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x6 0x2

#		sleep $ME_COLD_RESET_DELAY

	done



#	printf "   <<< IA32_MCG_CAP, ProcessorID from 0 to %u  >>> \n" $max_proc_id
#
#	ProcessorID=0
#	retry=0
#	while [ "$ProcessorID" -le $max_proc_id ]
#	do
#		param=$(($IA32_MCG_CAP))
#		param0=$(($param & 255))
#		param1=$(($param >> 8))
#		#echo "< ProcessorID=$ProcessorID Read >"
#		#printf "[issue] %.2x %.2x %.2x %.2x %s %s\n" $ProcessorID $param $param0 $param1 $user $passwd
#		#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MCG_CAP\n" $ProcessorID $param
#		#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
#		response=$(ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
#		comp=$?
#		return=$(echo $response | cut -d ' ' -f 4)
#
#		if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
#			#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
#			retry=$(($retry+1))
#			sleep 1
#		else
#			echo "$response"
#			ProcessorID=$(($ProcessorID+1))
#			retry=0
#		fi
#	done

	printf "   <<< IA32_MCG_STATUS, ProcessorID from 0 to %u  >>> \n" $max_proc_id

	ProcessorID=0
	retry=0
	while [ "$ProcessorID" -le $max_proc_id ]
	do
		param=$(($IA32_MCG_STATUS))
		param0=$(($param & 255))
		param1=$(($param >> 8))
		#echo "< ProcessorID=$ProcessorID Read >"
		#printf "[issue] %.2x %.2x %.2x %.2x %s %s\n" $ProcessorID $param $param0 $param1 $user $passwd
		#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MCG_STATUS\n" $ProcessorID $param
		#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
		cmdout=$($ME_UTIL $1 0xB8 0x40 0x57 0x01 0x00 0x30 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
    response=$(head -n 1 $ME_UTIL_LOG)
		comp=$?
		return=$(echo $response | cut -d ' ' -f 4)

		if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
			#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
			retry=$(($retry+1))
			sleep 1
		else
			echo "$response"
			ProcessorID=$(($ProcessorID+1))
			retry=0
		fi
	done



#	printf "   <<< IA32_MCG_CONTAIN, ProcessorID from 0 to %u  >>> \n" $max_proc_id
#
#	ProcessorID=0
#	retry=0
#	while [ "$ProcessorID" -le $max_proc_id ]
#	do
#		param=$(($IA32_MCG_CONTAIN))
#		param0=$(($param & 255))
#		param1=$(($param >> 8))
#		#echo "< ProcessorID=$ProcessorID Read >"
#		#printf "[issue] %.2x %.2x %.2x %.2x %s %s\n" $ProcessorID $param $param0 $param1 $user $passwd
#		#printf "[issue] ProcessorID=%u MSR_Address=0x%.4x IA32_MCG_CONTAIN\n" $ProcessorID $param
#		#printf "[issue] ipmitool -H %s -U %s -P %s -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 0x%.2x 0x05 0x09 0xb1 0x00 0x%.2x 0x%.2x 0x%.2x\n" $1 $user $passwd $2  $ProcessorID $param0 $param1
#		response=$(ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1)
#		comp=$?
#		return=$(echo $response | cut -d ' ' -f 4)
#
#		if ( ( [ "$return" = "80" ] || [ "$return" = "81" ] || [ "$return" = "82" ] || [ $comp != 0 ] ) &&  [ "$retry" -lt $max_retry ] ); then
#			#echo "retry command: ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x09 0xb1 0x00 $ProcessorID $param0 $param1"
#			retry=$(($retry+1))
#			sleep 1
#		else
#			echo "$response"
#	        ProcessorID=$(($ProcessorID+1))
#			retry=0
#		fi
#	done

	exit 1

fi

exit 0
