//simplerouter2.ned
//Created by Michael Ciavarella

//This ned file depicts a basic coaxial cable-based network.
//There are hosts, cable modems, a CMTS, upstream and aggregate routers, and a sink
//The aggregate and upstream routers have hosts attached to them to simulate
//	other network traffic.

package simple2;

import inet.linklayer.queue.DropTailQueue;
import inet.linklayer.queue.DocsisQueue;
import inet.linklayer.queue.PriorityScheduler;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.manetrouting.DSRUU;
import inet.nodes.ethernet.Eth1G;
import inet.nodes.inet.CMTS;
import inet.nodes.inet.Modem;
import inet.nodes.inet.Router;
import inet.nodes.inet.StandardHost;
import inet.util.PcapRecorder;
import ned.DatarateChannel;

//channels are currently at LOW
channel houseEth extends DatarateChannel
{
    double hlength @unit(m) = default(10m);
    delay = replaceUnit(hlength / 2e8, "s");
    datarate = 1Gbps;
    ber = 1/1e10;
}

channel downCableEth extends DatarateChannel
{
    double dclength @unit(m) = default(25000m);
    delay = replaceUnit(dclength / 2e8, "s");
    datarate = 20Mbps;
    ber = 1/1e10;
}


channel upCableEth extends DatarateChannel
{
    double uclength @unit(m) = default(25000m);
    delay = replaceUnit(uclength / 2e8, "s");
    datarate = 2Mbps;
    ber = 1/1e10;
}

channel upstreamEth extends DatarateChannel
{
    double ulength @unit(m) = default(1000m);
    delay = replaceUnit(ulength / 2e8, "s");
    datarate = 1Gbps;
    ber = 1/1e10;
}

channel downstreamEth extends DatarateChannel
{
    double dlength @unit(m) = default(1000m);
    delay = replaceUnit(dlength / 2e8, "s");
    datarate = 1Gbps;
    ber = 1/1e10;
}

channel uSinkEth extends DatarateChannel
{
    double uslength @unit(m) = default(804672m); //500 miles
    delay = replaceUnit(uslength / 2e8, "s");
    datarate = 1Gbps;
    ber = 1/1e10;
}

channel dSinkEth extends DatarateChannel
{
    double dslength @unit(m) = default(804672m); //500 miles
    delay = replaceUnit(dslength / 2e8, "s");
    datarate = 1Gbps;
    ber = 1/1e10;
}

network SimpleNet2
{
    parameters:
        int numHosts = default(1);
        int numGens = default(1);
        @display("bgb=694,352");
    submodules:
        modem[numHosts]: Modem {
            @display("p=203,219;i=device/modem");
        }
        host[numHosts]: StandardHost {
            @display("p=93,173");
        }
        sink: StandardHost {
            @display("p=484,44");
        }
        //dtqueue: DropTailQueue {
        //    @display("p=242,144");
        //}
        configurator: IPv4NetworkConfigurator {
            @display("p=79,32");
        }
        //pri: PriorityScheduler {
        //    @display("p=292,63");
        //}
        cmts: CMTS {
            @display("p=354,195;i=device/router");
            numCMs = numHosts;
        }
        ur: Router {
            @display("p=412,160");
        }
        ar: Router {
            @display("p=451,102");
        }
        upHost[numGens]: StandardHost {
            @display("p=438,258");
        }
        agHost[numGens]: StandardHost {
            @display("p=572,143");
        }
    //        host2: StandardHost {
    //            @display("p=194,189");
    //            numPingApps = 1;
    //        }
    //        router2: Router {
    //            @display("p=357,149");
    //        }
    //        host3: StandardHost {
    //            @display("p=396,250");
    //            numPingApps = 1;
    //        }
    //        host4: StandardHost {
    //            @display("p=478,228");
    //            numPingApps = 1;
    //        }
    connections:
        for i=0..numHosts-1 {
            host[i].ethg++ <--> houseEth <--> modem[i].ethg++;
        }

        for i=0..numHosts-1 {
            //modem[i].pppg++ <--> cableEth <--> cmts.pppg++;
            modem[i].pppg$o++ --> upCableEth --> cmts.pppg$i++;
            cmts.pppg$o++ --> downCableEth --> modem[i].pppg$i++;
        }
        cmts.pppg$o++ --> upstreamEth --> ur.pppg$i++;
        ur.pppg$o++ --> downstreamEth --> cmts.pppg$i++;
        
        ur.pppg$o++ --> upstreamEth --> ar.pppg$i++;
        ar.pppg$o++ --> downstreamEth --> ur.pppg$i++;
        
        ar.pppg$o++ --> uSinkEth --> sink.pppg$i++;
        sink.pppg$o++ --> dSinkEth -->ar.pppg$i++;
        
        for i=0..numGens-1 {
            upHost[i].pppg$o++ --> upstreamEth --> ur.pppg$i++;
            agHost[i].pppg$o++ --> upstreamEth --> ar.pppg$i++;
            ur.pppg$o++ --> downstreamEth --> upHost[i].pppg$i++;
            ar.pppg$o++ --> downstreamEth --> agHost[i].pppg$i++;
        }

        //sink.ethg$o++ --> Eth1G --> router.ethg$i++;

        // router.pppg$o++ --> Eth1G --> dtqueue.in;
        // dtqueue.out --> pri.in++;
        // pri.out --> r2.pppg$i++;
        //cmts.ethg++ <--> Eth1G <--> sink.ethg++;
        //dtqueue.out --> Eth1G --> sink.ethg$i++;
       // router.pppg++ <--> Eth1G <--> r2.pppg++;

}


