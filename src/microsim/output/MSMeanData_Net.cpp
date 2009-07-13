/****************************************************************************/
/// @file    MSMeanData_Net.cpp
/// @author  Daniel Krajzewicz
/// @date    Mon, 10.05.2004
/// @version $Id$
///
// Network state mean data collector for edges/lanes
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright 2001-2009 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <microsim/MSEdgeControl.h>
#include <microsim/MSEdge.h>
#include <microsim/MSLane.h>
#include <microsim/MSVehicle.h>
#include <utils/common/SUMOTime.h>
#include <utils/common/ToString.h>
#include <utils/iodevices/OutputDevice.h>
#include "MSMeanData_Net.h"
#include <limits>

#ifdef HAVE_MESOSIM
#include <microsim/MSGlobals.h>
#include <mesosim/MELoop.h>
#include <mesosim/MESegment.h>
#endif

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// method definitions
// ===========================================================================
// ---------------------------------------------------------------------------
// MSMeanData_Net::MSLaneMeanDataValues - methods
// ---------------------------------------------------------------------------
MSMeanData_Net::MSLaneMeanDataValues::MSLaneMeanDataValues(MSLane * const lane) throw()
        : MSMoveReminder(lane), sampleSeconds(0), nVehLeftLane(0), nVehEnteredLane(0),
        speedSum(0), haltSum(0), vehLengthSum(0),
        emitted(0) {}


MSMeanData_Net::MSLaneMeanDataValues::~MSLaneMeanDataValues() throw() {
}


void
MSMeanData_Net::MSLaneMeanDataValues::reset() throw() {
    sampleSeconds = 0.;
    nVehLeftLane = 0;
    nVehEnteredLane = 0;
    speedSum = 0;
    haltSum = 0;
    vehLengthSum = 0;
    emitted = 0;
}


bool
MSMeanData_Net::MSLaneMeanDataValues::isStillActive(MSVehicle& veh, SUMOReal oldPos, SUMOReal newPos, SUMOReal newSpeed) throw() {
    bool ret = true;
    SUMOReal l = veh.getVehicleType().getLength();
    SUMOReal fraction = 1.;
    if (oldPos<0&&newSpeed!=0) {
        fraction = (oldPos+SPEED2DIST(newSpeed)) / newSpeed;
        ++nVehEnteredLane;
    }
    if (oldPos+SPEED2DIST(newSpeed)>getLane()->length()&&newSpeed!=0) {
        fraction -= (oldPos+SPEED2DIST(newSpeed) - getLane()->length()) / newSpeed;
        ++nVehLeftLane;
        ret = false;
    }
    if(fraction<0) {
        MsgHandler::getErrorInstance()->inform("Negative vehicle step fraction on lane '" + getLane()->getID() + "'.");
        return false;
    }
    if(fraction==0) {
        return false;
    }
    sampleSeconds += fraction;
    speedSum += newSpeed * fraction;
    vehLengthSum += l * fraction;
    if (newSpeed<0.1) { // !!! swell
        haltSum++;
    }
    return ret;
}


bool
MSMeanData_Net::MSLaneMeanDataValues::isActivatedByEmitOrLaneChange(MSVehicle& veh, bool isEmit) throw() {
    ++emitted;
    SUMOReal l = veh.getVehicleType().getLength();
    SUMOReal fraction = 1.;
    if (veh.getPositionOnLane()+l>getLane()->length()) {
        fraction = l - (getLane()->length()-veh.getPositionOnLane());
    }
    if(fraction<0) {
        MsgHandler::getErrorInstance()->inform("Negative vehicle step fraction on lane '" + getLane()->getID() + "'.");
        return false;
    }
    if(fraction==0) {
        return false;
    }
    sampleSeconds += fraction;
    speedSum += veh.getSpeed() * fraction;
    vehLengthSum += l * fraction;
    if (veh.getSpeed()<0.1) { // !!! swell
        haltSum++;
    }
    return true;
}



// ---------------------------------------------------------------------------
// MSMeanData_Net - methods
// ---------------------------------------------------------------------------
MSMeanData_Net::MSMeanData_Net(const std::string &id,
                               MSEdgeControl &ec,
                               SUMOTime dumpBegin,
                               SUMOTime dumpEnd,
                               bool useLanes,
                               bool withEmptyEdges, bool withEmptyLanes) throw()
        : myID(id),
        myAmEdgeBased(!useLanes), myDumpBegin(dumpBegin), myDumpEnd(dumpEnd),
        myDumpEmptyEdges(withEmptyEdges), myDumpEmptyLanes(withEmptyLanes) {
    const std::vector<MSEdge*> &edges = ec.getEdges();
    for (std::vector<MSEdge*>::const_iterator e = edges.begin(); e != edges.end(); ++e) {
        std::vector<MSLaneMeanDataValues*> v;
#ifdef HAVE_MESOSIM
        if (MSGlobals::gUseMesoSim) {
            MESegment *s = MSGlobals::gMesoNet->getSegmentForEdge(*e);
            while (s!=0) {
                v.push_back(s->addDetector(this));
                s = s->getNextSegment();
            }
        } else {
#endif
            const MSEdge::LaneCont * const lanes = (*e)->getLanes();
            MSEdge::LaneCont::const_iterator lane;
            for (lane = lanes->begin(); lane != lanes->end(); ++lane) {
                v.push_back(new MSLaneMeanDataValues(*lane));
            }
#ifdef HAVE_MESOSIM
        }
#endif
        myMeasures.push_back(v);
        myEdges.push_back(*e);
    }
}


MSMeanData_Net::~MSMeanData_Net() throw() {}


void
MSMeanData_Net::resetOnly(SUMOTime stopTime) throw() {
#ifdef HAVE_MESOSIM
    if (MSGlobals::gUseMesoSim) {
        std::vector<MSEdge*>::iterator edge = myEdges.begin();
        for (std::vector<std::vector<MSLaneMeanDataValues*> >::const_iterator i=myMeasures.begin(); i!=myMeasures.end(); ++i, ++edge) {
            MESegment *s = MSGlobals::gMesoNet->getSegmentForEdge(*edge);
            for (std::vector<MSLaneMeanDataValues*>::const_iterator j=(*i).begin(); j!=(*i).end(); ++j) {
                s->prepareMeanDataForWriting(*(*j), (SUMOReal) stopTime);
                (*j)->reset();
                s = s->getNextSegment();
            }
        }
    } else {
#endif
        for (std::vector<std::vector<MSLaneMeanDataValues*> >::const_iterator i=myMeasures.begin(); i!=myMeasures.end(); ++i) {
            for (std::vector<MSLaneMeanDataValues*>::const_iterator j=(*i).begin(); j!=(*i).end(); ++j) {
                (*j)->reset();
            }
        }
#ifdef HAVE_MESOSIM
    }
#endif
}


void
MSMeanData_Net::writeEdge(OutputDevice &dev,
                          const std::vector<MSLaneMeanDataValues*> &edgeValues,
                          MSEdge *edge, SUMOTime startTime, SUMOTime stopTime) throw(IOError) {
#ifdef HAVE_MESOSIM
    if (MSGlobals::gUseMesoSim) {
        MESegment *s = MSGlobals::gMesoNet->getSegmentForEdge(edge);
        SUMOReal flowOut = 0;
        SUMOReal flowMean = 0;
        SUMOReal meanDensityS = 0;
        SUMOReal meanOccupancyS = 0;
        SUMOReal meanSpeedS = 0;
        SUMOReal traveltimeS = 0;
        unsigned noStopsS = 0;
        unsigned noEmissionsS = 0;
        unsigned noLeftS = 0;
        unsigned noEnteredS = 0;
        SUMOReal nVehS = 0;
        SUMOReal absLen = 0;
        int noSegments = 0;
        int noNotEmpty = 0;
        bool isFirst = true;
        while (s!=0) {
            SUMOReal traveltime = -42;
            SUMOReal meanSpeed = -43;
            SUMOReal meanDensity = -45;
            SUMOReal meanOccupancy = -46;
            MSLaneMeanDataValues& meanData = s->getDetectorData(this);
            s->prepareMeanDataForWriting(meanData, (SUMOReal) stopTime);
            conv(meanData, (stopTime-startTime),
                 s->getLength(), s->getMaxSpeed(),
                 traveltime, meanSpeed, meanDensity, meanOccupancy);
            meanDensityS += meanDensity;
            meanOccupancyS += meanOccupancy;
            traveltimeS += traveltime;
            noStopsS += meanData.haltSum;
            noEmissionsS += meanData.emitted;
            if (isFirst) {
                noEnteredS += meanData.nVehEnteredLane;
            }
            nVehS += meanData.sampleSeconds;
            flowMean += meanData.nVehLeftLane;
            if (meanData.sampleSeconds>0) {
                meanSpeedS += meanSpeed;
                noNotEmpty++;
            }
            noSegments++;
            absLen += s->getLength();
            if (s->getNextSegment()==0) {
                flowOut = meanData.nVehLeftLane;
                noLeftS = meanData.nVehLeftLane;
            }
            s = s->getNextSegment();
            meanData.reset();
            isFirst = false;
        }
        if (myDumpEmptyEdges||nVehS>0) {
            meanDensityS = meanDensityS / (SUMOReal) noSegments;
            meanOccupancyS = meanOccupancyS / (SUMOReal) noSegments / (SUMOReal) edge->nLanes();
            meanSpeedS = noNotEmpty!=0 ? meanSpeedS / (SUMOReal) noNotEmpty : 0;
            if (nVehS==0) {
                meanSpeedS = MSGlobals::gMesoNet->getSegmentForEdge(edge)->getMaxSpeed();
            } else {
                if (meanSpeedS>0) {
                    traveltimeS = absLen / meanSpeedS;
                } else {
                    traveltimeS = (SUMOReal) 1000000.00;
                }
            }
            flowMean /= (SUMOReal) noSegments;
            dev<<"      <edge id=\""<<edge->getID()<<
            "\" traveltime=\""<<traveltimeS<<
            "\" sampledSeconds=\""<< nVehS <<
            "\" density=\""<<meanDensityS<<
            "\" occupancy=\""<<meanOccupancyS<<
            //"\" noStops=\""<<noStopsS<<
            "\" speed=\""<<meanSpeedS<<
            "\" entered=\""<<noEnteredS<<
            "\" emitted=\""<<noEmissionsS<<
            "\" left=\""<<noLeftS<<
            "\" flowMean=\""<<(flowMean*3600./((SUMOReal)(stopTime-startTime)))<<
            "\" flow=\""<<(flowOut*3600./((SUMOReal)(stopTime-startTime)))<<
            "\"/>\n";
        }
    } else {
#endif
        std::vector<MSLaneMeanDataValues*>::const_iterator lane;
        if (!myAmEdgeBased) {
            bool writeCheck = myDumpEmptyEdges;
            if (!writeCheck) {
                for (lane = edgeValues.begin(); lane != edgeValues.end(); ++lane) {
                    if ((*lane)->sampleSeconds>0) {
                        writeCheck = true;
                        break;
                    }
                }
            }
            if (writeCheck) {
                dev<<"      <edge id=\""<<edge->getID()<<"\">\n";
                for (lane = edgeValues.begin(); lane != edgeValues.end(); ++lane) {
                    writeLane(dev, *(*lane), startTime, stopTime);
                }
                dev<<"      </edge>\n";
            }
        } else {
            SUMOReal traveltimeS = 0;
            SUMOReal meanSpeedS = 0;
            SUMOReal meanDensityS = 0;
            unsigned noStopsS = 0;
            SUMOReal nVehS = 0;
            SUMOReal meanOccupancyS = 0;
            unsigned noEmissionsS = 0;
            unsigned noLeftS = 0;
            unsigned noEnteredS = 0;
            SUMOReal noLanes = 0;
            for (lane = edgeValues.begin(); lane != edgeValues.end(); ++lane, noLanes += 1) {
                MSLaneMeanDataValues& meanData = *(*lane);
                // calculate mean data
                SUMOReal traveltime = -42;
                SUMOReal meanSpeed = -43;
                SUMOReal meanDensity = -45;
                SUMOReal meanOccupancy = -46;
                conv(meanData, (stopTime-startTime),
                     (*lane)->getLane()->length(), (*lane)->getLane()->maxSpeed(),
                     traveltime, meanSpeed, meanDensity, meanOccupancy);
                traveltimeS += traveltime;
                meanSpeedS += meanSpeed;
                meanDensityS += meanDensity;
                meanOccupancyS += meanOccupancy;
                noStopsS += meanData.haltSum;
                noEmissionsS += meanData.emitted;
                noLeftS += meanData.nVehLeftLane;
                noEnteredS += meanData.nVehEnteredLane;
                nVehS += meanData.sampleSeconds;
                meanData.reset();
            }
            if (myDumpEmptyEdges||nVehS>0) {
                dev<<"      <edge id=\""<<edge->getID()<<
                "\" traveltime=\""<<(traveltimeS/noLanes)<<
                "\" sampledSeconds=\""<< nVehS <<
                "\" density=\""<<meanDensityS<<
                "\" occupancy=\""<<(meanOccupancyS/noLanes)<<
                "\" noStops=\""<<noStopsS<<
                "\" speed=\""<<(meanSpeedS/noLanes)<<
                "\" entered=\""<<noEnteredS<<
                "\" emitted=\""<<noEmissionsS<<
                "\" left=\""<<noLeftS<<
                "\"/>\n";
            }
        }
#ifdef HAVE_MESOSIM
    }
#endif
}


void
MSMeanData_Net::writeLane(OutputDevice &dev,
                          MSLaneMeanDataValues &laneValues,
                          SUMOTime startTime, SUMOTime stopTime) throw(IOError) {
    if (myDumpEmptyLanes||laneValues.sampleSeconds>0) {
        // calculate mean data
        SUMOReal traveltime = -42;
        SUMOReal meanSpeed = -43;
        SUMOReal meanDensity = -44;
        SUMOReal meanOccupancy = -45;
        conv(laneValues, (stopTime-startTime),
             laneValues.getLane()->length(), laneValues.getLane()->maxSpeed(),
             traveltime, meanSpeed, meanDensity, meanOccupancy);
        dev<<"         <lane id=\""<<laneValues.getLane()->getID()<<
        "\" traveltime=\""<<traveltime<<
        "\" sampledSeconds=\""<< laneValues.sampleSeconds <<
        "\" density=\""<<meanDensity<<
        "\" occupancy=\""<<meanOccupancy<<
        "\" noStops=\""<<laneValues.haltSum<<
        "\" speed=\""<<meanSpeed<<
        "\" entered=\""<<laneValues.nVehEnteredLane<<
        "\" emitted=\""<<laneValues.emitted<<
        "\" left=\""<<laneValues.nVehLeftLane<<
        "\"/>\n";
    }
    laneValues.reset();
}


void
MSMeanData_Net::writeXMLOutput(OutputDevice &dev,
                               SUMOTime startTime, SUMOTime stopTime) throw(IOError) {
    // check whether this dump shall be written for the current time
    if (myDumpBegin < stopTime && myDumpEnd-DELTA_T >= startTime) {
        dev<<"   <interval begin=\""<<startTime<<"\" end=\""<<
        stopTime<<"\" "<<"id=\""<<myID<<"\">\n";
        std::vector<MSEdge*>::iterator edge = myEdges.begin();
        for (std::vector<std::vector<MSLaneMeanDataValues*> >::const_iterator i=myMeasures.begin(); i!=myMeasures.end(); ++i, ++edge) {
            writeEdge(dev, (*i), *edge, startTime, stopTime);
        }
        dev<<"   </interval>\n";
    } else {
        resetOnly(stopTime);
    }
}


void
MSMeanData_Net::writeXMLDetectorProlog(OutputDevice &dev) const throw(IOError) {
    dev.writeXMLHeader("netstats");
}


/****************************************************************************/

