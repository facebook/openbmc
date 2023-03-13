from api_emulator.redfish.EventService_api import EventServiceAPI, CreateEventService
from api_emulator.redfish.Chassis_api import ChassisCollectionAPI, ChassisAPI, CreateChassis
from api_emulator.redfish.ComputerSystem_api import ComputerSystemCollectionAPI, ComputerSystemAPI, CreateComputerSystem
from api_emulator.redfish.Manager_api import ManagerCollectionAPI, ManagerAPI, CreateManager
from api_emulator.redfish.pcie_switch_api import PCIeSwitchesAPI, PCIeSwitchAPI
from api_emulator.redfish.eg_resource_api import EgResourceCollectionAPI, EgResourceAPI, CreateEgResource
from api_emulator.redfish.power_api import CreatePower
from api_emulator.redfish.thermal_api import CreateThermal
from api_emulator.redfish.ResetAction_api import ResetAction_API
from api_emulator.redfish.ResetActionInfo_api import ResetActionInfo_API
from api_emulator.redfish.processor import CreateProcessor
from api_emulator.redfish.memory import CreateMemory
from api_emulator.redfish.simplestorage import CreateSimpleStorage
from api_emulator.redfish.ethernetinterface import CreateEthernetInterface

import g

from api_emulator.redfish.ResourceBlock_api import CreateResourceBlock
from api_emulator.redfish.ResourceZone_api import CreateResourceZone

# In Python 3, xrange() is not supported.  The usage in this file uses the
# Python 3 interpretation of range()
#   python 2 to python 3 equivalencies
#   xrange(10) -> range(10)
#   range(10)  -> list(range(10)
#
def create_resources(template, chassis, suffix, suffix_id):
    resource_ids={'Processors':[],'Memory':[],'SimpleStorage':[],'EthernetInterfaces':[]}
    proc_count=mem_count=dsk_count=eth_count=0
    for proc in template['Processors']:
        for k in range(proc.get('Count', 1)):
            proc_id=proc['Id'].format(proc_count)
            proc_count+=1
            resource_ids['Processors'].append(proc_id)
            CreateProcessor(rb=g.rest_base, suffix=suffix, processor_id=proc_id,
                            suffix_id=suffix_id, chassis_id=chassis, totalcores=proc.get('TotalCores', 8),
                            maxspeedmhz=proc.get('MaxSpeedMHz', 2200))
    for mem in template['Memory']:
        memtype = mem.get('MemoryType', 'DRAM')
        opmodes = ['PMEM'] if 'NV' in memtype else ['Volatile']
        for k in range(mem.get('Count', 1)):
            mem_id=mem['Id'].format(mem_count)
            mem_count+=1
            resource_ids['Memory'].append(mem_id)
            CreateMemory(rb=g.rest_base, suffix=suffix, memory_id=mem_id,
                         suffix_id=suffix_id, chassis_id=chassis, capacitymb=mem.get('CapacityMiB', 8192),
                         type=memtype, operatingmodes=opmodes)

    for dsk in template['SimpleStorage']:
        for k in range(dsk.get('Count', 1)):
            dsk_id=dsk['Id'].format(dsk_count)
            dsk_count+=1
            resource_ids['SimpleStorage'].append(dsk_id)
            capacitygb = int(dsk['Devices'].get('CapacityBytes',
                                                512 * 1024 ** 3) / 1024 ** 3)
            drives = dsk['Devices'].get('Count', 1)
            CreateSimpleStorage(rb=g.rest_base, suffix=suffix, storage_id=dsk_id,
                                suffix_id=suffix_id, chassis_id=chassis, capacitygb=capacitygb,
                                drives=drives)

    for eth in template['EthernetInterfaces']:
        for k in range(eth.get('Count', 1)):
            eth_id=eth['Id'].format(eth_count)
            eth_count+=1
            resource_ids['EthernetInterfaces'].append(eth_id)
            CreateEthernetInterface(rb=g.rest_base, suffix=suffix, nic_id=eth_id,
                                    suffix_id=suffix_id, chassis_id=chassis,
                                    speedmbps=eth.get('SpeedMbps', 1000))
    return resource_ids

def populate(cfg):
    #cfg = 10
    if type(cfg) is int:
        return n_populate(cfg)
    cs_count = 0
    rb_count = 0
    chassis_count = 0
    zones = {}
    for chassi_template in cfg['Chassis']:
        for i in range(chassi_template.get('Count', 1)):
            chassis_count += 1
            chassis = chassi_template['Id'].format(chassis_count)
            bmc = 'BMC-{}'.format(chassis_count)
            sys_ids = []
            rb_ids = []
            for compsys_template in chassi_template['Links'].get('ComputerSystems',[]):
                for j in range(compsys_template.get('Count', 1)):
                    cs_count += 1
                    compSys = compsys_template['Id'].format(cs_count)
                    sys_ids.append(compSys)
                    CreateComputerSystem(
                        resource_class_kwargs={'rb': g.rest_base, 'linkChassis': [chassis], 'linkMgr': bmc}).put(
                        compSys)
                    create_resources(compsys_template, chassis, 'Systems', compSys)

            for rb_template in chassi_template['Links'].get('ResourceBlocks',[]):
                for j in range(rb_template.get('Count', 1)):
                    rb_count += 1
                    rb_id = rb_template['Id'].format(rb_count)
                    rb_ids.append(rb_id)
                    rb_zones = rb_template['ResourceZones']
                    for zone in rb_zones:
                        if zone not in zones:
                            zones[zone]=[]
                        zones[zone].append(rb_id)
                    rb=CreateResourceBlock(resource_class_kwargs={'rb': g.rest_base})
                    rb.put(rb_id)
                    rb.post(g.rest_base,rb_id,'linkChassis',chassis)
                    [rb.post(g.rest_base,rb_id,'linkZone',x) for x in rb_zones]
                    resource_ids=create_resources(rb_template, chassis, 'CompositionService/ResourceBlocks', rb_id)
                    for resource_name in resource_ids:
                        for resource_id in resource_ids[resource_name]:
                            rb.post(g.rest_base,rb_id,resource_name,resource_id)

            CreateChassis(resource_class_kwargs={
                'rb': g.rest_base, 'linkSystem': sys_ids, 'linkResourceBlocks':rb_ids, 'linkMgr': bmc}).put(chassis)
            CreatePower(resource_class_kwargs={'rb': g.rest_base, 'ch_id': chassis}).put(chassis)
            CreateThermal(resource_class_kwargs={'rb': g.rest_base, 'ch_id': chassis}).put(chassis)
            CreateManager(resource_class_kwargs={
                'rb': g.rest_base, 'linkSystem': sys_ids, 'linkChassis': chassis, 'linkInChassis': chassis}).put(bmc)

    for zone in zones:
        z=CreateResourceZone(resource_class_kwargs={'rb': g.rest_base})
        z.put(zone)
        [z.post(g.rest_base,zone,'ResourceBlocks',x) for x in zones[zone]]


def n_populate(num):
    # populate with some example infrastructure
    for i in range(num):
        chassis = 'Chassis-{0}'.format(i + 1)
        compSys = 'System-{0}'.format(i + 1)
        bmc = 'BMC-{0}'.format(i + 1)
        # create chassi
        CreateChassis(resource_class_kwargs={
            'rb': g.rest_base, 'linkSystem': [compSys], 'linkMgr': bmc}).put(chassis)
        # create chassi subordinate sustems
        CreatePower(resource_class_kwargs={'rb': g.rest_base, 'ch_id': chassis}).put(chassis)
        CreateThermal(resource_class_kwargs={'rb': g.rest_base, 'ch_id': chassis}).put(chassis)
        # create ComputerSystem
        CreateComputerSystem(resource_class_kwargs={
            'rb': g.rest_base, 'linkChassis': [chassis], 'linkMgr': bmc}).put(compSys)
        # subordinates, note that .put does not need to be called here
        ResetAction_API(resource_class_kwargs={'rb': g.rest_base, 'sys_id': compSys})
        ResetActionInfo_API(resource_class_kwargs={'rb': g.rest_base, 'sys_id': compSys})
        CreateProcessor(rb=g.rest_base, suffix='System', processor_id='CPU0', suffix_id=compSys, chassis_id=chassis)
        CreateProcessor(rb=g.rest_base, suffix='System', processor_id='CPU1', suffix_id=compSys, chassis_id=chassis)
        CreateMemory(rb=g.rest_base, suffix='System', memory_id='DRAM1', suffix_id=compSys, chassis_id=chassis)
        CreateMemory(rb=g.rest_base, suffix='System', memory_id='NVRAM1', suffix_id=compSys, chassis_id=chassis,
                     capacitymb=65536, devicetype='DDR4', type='NVDIMM_N', operatingmodes=['PMEM'])
        CreateSimpleStorage(rb=g.rest_base, suffix='System', suffix_id=compSys, storage_id='controller-1', drives=2,
                            capacitygb=512, chassis_id=chassis)
        CreateSimpleStorage(rb=g.rest_base, suffix='System', suffix_id=compSys, storage_id='controller-2', drives=2,
                            capacitygb=512, chassis_id=chassis)
        CreateEthernetInterface(rb=g.rest_base, suffix='System', suffix_id=compSys, nic_id='NIC-1',
                                speedmbps=40000, vlan_id=4095, chassis_id=chassis)
        CreateEthernetInterface(rb=g.rest_base, suffix='System', suffix_id=compSys, nic_id='NIC-2',
                                speedmbps=40000, vlan_id=4095, chassis_id=chassis)
        # create manager
        CreateManager(resource_class_kwargs={
            'rb': g.rest_base, 'linkSystem': compSys, 'linkChassis': chassis, 'linkInChassis': chassis}).put(bmc)

        # create Resource Block

        RB = 'RB-{0}'.format(i + 1)
        config = CreateResourceBlock(resource_class_kwargs={'rb': g.rest_base})
        config.put(RB)

        config.post(g.rest_base, RB, "linkSystem", "CS_%d" % i)
        config.post(g.rest_base, RB, "linkChassis", "Chassis-%d" % i)
        config.post(g.rest_base, RB, "linkZone", "ResourceZone-%d" % i)

        for j in range(2):
            # create ResourceBlock Processor (1)
            CreateProcessor(rb=g.rest_base, suffix='CompositionService/ResourceBlocks', processor_id='CPU-%d' % (i + 1),
                            suffix_id=RB, chassis_id=chassis)
            config.post(g.rest_base, RB, "Processors", 'CPU-%d' % (j + 1))

            # create ResourceBlock Memory (1)
            CreateMemory(rb=g.rest_base, suffix='CompositionService/ResourceBlocks', memory_id='MEM-%d' % (i + 1),
                         suffix_id=RB, chassis_id=chassis)
            config.post(g.rest_base, RB, "Memory", 'MEM-%d' % (j + 1))
            CreateMemory(rb=g.rest_base, suffix='CompositionService/ResourceBlocks', memory_id='MEM-%d' % (i + 3),
                         suffix_id=RB, chassis_id=chassis,
                         capacitymb=65536, devicetype='DDR4', type='NVDIMM_N', operatingmodes='PMEM')
            config.post(g.rest_base, RB, "Memory", 'MEM-%d' % (j + 2))

            CreateSimpleStorage(rb=g.rest_base, suffix='CompositionService/ResourceBlocks', suffix_id=RB,
                                storage_id='SS-%d' % (j + 1), drives=2,
                                capacitygb=512, chassis_id=chassis)
            config.post(g.rest_base, RB, "SimpleStorage", 'SS-%d' % (j + 1))

            CreateEthernetInterface(rb=g.rest_base, suffix='CompositionService/ResourceBlocks', suffix_id=RB,
                                    nic_id='EI-%d' % (j + 1),
                                    speedmbps=40000, vlan_id=4095, chassis_id=chassis)
            config.post(g.rest_base, RB, "EthernetInterfaces", 'EI-%d' % (j + 1))

        # create Resource Zone

        RZ = 'RZ-{0}'.format(i + 1)

        config = CreateResourceZone(resource_class_kwargs={'rb': g.rest_base})

        config.put(RZ)
        config.post(g.rest_base, RZ, "ResourceBlocks", 'RB-%d' % (j + 1))
        config.post(g.rest_base, RZ, "ResourceBlocks", 'RB-%d' % (j + 2))
