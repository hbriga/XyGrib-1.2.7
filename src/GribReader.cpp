/**********************************************************************
XyGrib: meteorological GRIB file viewer
Copyright (C) 2008-2012 - Jacques Zaninetti - http://www.zygrib.org

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#include <cassert>

#include "GribReader.h"
#include "Util.h"
#include "DataQString.h"
#include "Therm.h"

//-------------------------------------------------------------------------------
GribReader::GribReader()
{
    ok = false;
	hasAltitude = false;
	ambiguousHeader = false;
	dewpointDataStatus = NO_DATA_IN_FILE;
	xmin =  1e300;
	xmax = -1e300;
	ymin =  1e300;
	ymax = -1e300;
}
//-------------------------------------------------------------------------------
void GribReader::openFile (const QString &fname, int nbrecs)
{
	continueDownload = true;
	setAllDataCenterModel.clear();
	setAllDates.clear ();
	setAllDataCode.clear ();
	
    if (!fname.isEmpty()) {
        openFilePriv (fname, nbrecs);
    }
    else {
        clean_all_vectors();
    }
}
//-------------------------------------------------------------------------------
GribReader::~GribReader()
{
// 	DBGS("Destroy GribReader");
    clean_all_vectors();
}
//-------------------------------------------------------------------------------
void GribReader::clean_all_vectors ()
{
	for (auto const &it : mapGribRecords) {
		std::vector<GribRecord *> *ls = it.second;
		clean_vector( *ls );
		delete ls;
	}
	mapGribRecords.clear();
}
//-------------------------------------------------------------------------------
void GribReader::clean_vector (std::vector<GribRecord *> &ls)
{
    for (auto & l : ls) {
        delete l;
        l = NULL;
    }
    ls.clear();
}
//---------------------------------------------------------------------------------
void GribReader::storeRecordInMap (GribRecord *rec)
{
    if (rec==nullptr || !rec->isOk())
		return;
//    DBG ("%g %g   %g %g", rec->getXmin(),rec->getXmax(), getYmin(),getYmax());

	auto it = mapGribRecords.find(rec->getKey());
	if (it == mapGribRecords.end())
	{
		mapGribRecords[rec->getKey()] = new std::vector<GribRecord *>;
		assert(mapGribRecords[rec->getKey()]);
	}
	
	mapGribRecords [rec->getKey()]->push_back (rec);
	
	if (xmin > rec->getXmin()) xmin = rec->getXmin();
	if (xmax < rec->getXmax()) xmax = rec->getXmax();
	if (ymin > rec->getYmin()) ymin = rec->getYmin();
	if (ymax < rec->getYmax()) ymax = rec->getYmax();
	
	if (rec->isOrientationAmbiguous()) {
		ambiguousHeader = true;
	}
	
	// Update list of records types
	setAllDataCode.insert (rec->getDataCode());
	setAllDataCenterModel.insert (rec->getDataCenterModel());
	
	if (rec->getLevelType()==LV_ISOBARIC
			&& (   rec->getLevelValue()==850
				|| rec->getLevelValue()==700
				|| rec->getLevelValue()==500
				|| rec->getLevelValue()==300 
				|| rec->getLevelValue()==200 
				)
	) {
		hasAltitude = true;
	}
}
//---------------------------------------------------------------------------------
void GribReader::readAllGribRecords (int nbrecs)
{
    //--------------------------------------------------------
    // Lecture de l'ensemble des GribRecord du fichier
    // et stockage dans les listes appropri??es.
    //--------------------------------------------------------
    GribRecord *rec = nullptr;
    int id = 0;
    time_t firstdate = -1;
	ok = false;
	bool eof;
    do {
		if (id%4 == 1)
			emit valueChanged ((int)(100.0*id/nbrecs));
		
		id ++;

		delete rec;
        rec = new GribRecord(file, id);
        assert(rec);
        
		if (!rec->isOk())
        	break;

        eof = rec->isEof();

		if (rec->isDataKnown())
        {
			if (firstdate== -1)
				firstdate = rec->getRecordCurrentDate();
//            DBG("%d %d %d %d", rec->getDataType(),rec->getLevelType(), rec->getLevelValue(), rec->getRecordCurrentDate());
			if (//-----------------------------------------
				(rec->getDataType()==GRB_PRESSURE_MSL
					&& rec->getLevelType()==LV_MSL && rec->getLevelValue()==0)
			//-----------------------------------------
			|| ( (rec->getDataType()==GRB_PRESSURE)
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			//-----------------------------------------
			|| ( (rec->getDataType()==GRB_TMIN || rec->getDataType()==GRB_TMAX)
					&& rec->getLevelType()==LV_ABOV_GND && rec->getLevelValue()==2)
			//-----------------------------------------
			|| ( rec->getDataType()==GRB_DEWPOINT
					&& rec->getLevelType()==LV_ABOV_GND && rec->getLevelValue()==2)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_TEMP
					&& rec->getLevelType()==LV_ABOV_GND && rec->getLevelValue()==2)
			|| (rec->getDataType()==GRB_TEMP
					&& rec->getLevelType()==LV_ABOV_GND && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_TEMP
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_TEMP
					&& rec->getLevelType()==LV_ISOBARIC
					&& (   rec->getLevelValue()==1000
						|| rec->getLevelValue()==975
						|| rec->getLevelValue()==950
						|| rec->getLevelValue()==925
						|| rec->getLevelValue()==900
						|| rec->getLevelValue()==875
						|| rec->getLevelValue()==850
						|| rec->getLevelValue()==825
						|| rec->getLevelValue()==800
						|| rec->getLevelValue()==775
						|| rec->getLevelValue()==750
						|| rec->getLevelValue()==725
						|| rec->getLevelValue()==700
						|| rec->getLevelValue()==650
						|| rec->getLevelValue()==600
						|| rec->getLevelValue()==550
						|| rec->getLevelValue()==500
						|| rec->getLevelValue()==400
						|| rec->getLevelValue()==300 
						|| rec->getLevelValue()==200 
						|| rec->getLevelValue()==100 ) )
			//-----------------------------------------
			// Current
			//-----------------------------------------
			|| ( (rec->getDataType()==GRB_CUR_VX || rec->getDataType()==GRB_CUR_VY
			      || rec->getDataType()==GRB_CUR_DIR || rec->getDataType()==GRB_CUR_SPEED)
                    /*&& rec->getLevelType()==LV_GND_SURF
                    && rec->getLevelValue()==0*/ )
			//-----------------------------------------
			// Wind
			//-----------------------------------------
			|| ( (rec->getDataType()==GRB_WIND_VX || rec->getDataType()==GRB_WIND_VY
			    || rec->getDataType()==GRB_WIND_DIR || rec->getDataType()==GRB_WIND_SPEED)
					&& rec->getLevelType()==LV_ABOV_GND
					&& (   rec->getLevelValue()==1
						|| rec->getLevelValue()==2
						|| rec->getLevelValue()==3
						|| rec->getLevelValue()==10 ) )
			|| ( (rec->getDataType()==GRB_WIND_VX || rec->getDataType()==GRB_WIND_VY
			    || rec->getDataType()==GRB_WIND_DIR || rec->getDataType()==GRB_WIND_SPEED)
					&& rec->getLevelType()==LV_MSL
					&& rec->getLevelValue()==0 )
			|| ( (rec->getDataType()==GRB_WIND_VX || rec->getDataType()==GRB_WIND_VY
			    || rec->getDataType()==GRB_WIND_DIR || rec->getDataType()==GRB_WIND_SPEED)
					&& rec->getLevelType()==LV_GND_SURF)
			|| ( (rec->getDataType()==GRB_WIND_VX || rec->getDataType()==GRB_WIND_VY
			    || rec->getDataType()==GRB_WIND_DIR || rec->getDataType()==GRB_WIND_SPEED)
					&& rec->getLevelType()==LV_ISOBARIC
					&& (   rec->getLevelValue()==925
						|| rec->getLevelValue()==1000
						|| rec->getLevelValue()==800
						|| rec->getLevelValue()==600
						|| rec->getLevelValue()==400
						|| rec->getLevelValue()==100
						|| rec->getLevelValue()==850
						|| rec->getLevelValue()==700
						|| rec->getLevelValue()==500
						|| rec->getLevelValue()==300 
						|| rec->getLevelValue()==200 ) )
			//-----------------------------------------
			//-----------------------------------------
			|| (rec->getDataType()==GRB_HUMID_SPEC
					&& rec->getLevelType()==LV_ISOBARIC
					&& (   rec->getLevelValue()==925
						|| rec->getLevelValue()==850
						|| rec->getLevelValue()==700
						|| rec->getLevelValue()==600
						|| rec->getLevelValue()==500
						|| rec->getLevelValue()==400
						|| rec->getLevelValue()==300 
						|| rec->getLevelValue()==200 ) )
			//-----------------------------------------
			|| (rec->getDataType()==GRB_HUMID_SPEC
					&& rec->getLevelType()==LV_ABOV_GND
					&& (   rec->getLevelValue()==2 ) )
			//-----------------------------------------
			|| (rec->getDataType()==GRB_GEOPOT_HGT
					&& rec->getLevelType()==LV_ISOTHERM0 && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_GEOPOT_HGT
					&& rec->getLevelType()==LV_ISOBARIC
					&& (   rec->getLevelValue()==925
						|| rec->getLevelValue()==850
						|| rec->getLevelValue()==700
						|| rec->getLevelValue()==600
						|| rec->getLevelValue()==500
						|| rec->getLevelValue()==400
						|| rec->getLevelValue()==300 
						|| rec->getLevelValue()==200 ) )
			//-----------------------------------------
			|| (rec->getDataType()==GRB_PRECIP_TOT
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_PRECIP_RATE
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_SNOW_DEPTH
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_SNOW_CATEG
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_FRZRAIN_CATEG
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_CLOUD_TOT
					&& rec->getLevelType()==LV_ATMOS_ALL && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_CLOUD_TOT
					&& rec->getLevelType()==LV_CLOUD_LOW_LAYER && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_CLOUD_TOT
					&& rec->getLevelType()==LV_CLOUD_MID_LAYER && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_CLOUD_TOT
					&& rec->getLevelType()==LV_CLOUD_HIG_LAYER && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_PRESSURE
					&& rec->getLevelType()==LV_CLOUD_LOW_TOP && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_PRESSURE
					&& rec->getLevelType()==LV_CLOUD_MID_TOP && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_PRESSURE
					&& rec->getLevelType()==LV_CLOUD_HIG_TOP && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_PRESSURE
					&& rec->getLevelType()==LV_CLOUD_LOW_BOTTOM && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_PRESSURE
					&& rec->getLevelType()==LV_CLOUD_MID_BOTTOM && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_PRESSURE
					&& rec->getLevelType()==LV_CLOUD_HIG_BOTTOM && rec->getLevelValue()==0)
			//-----------------------------------------
			|| (rec->getDataType()==GRB_HUMID_REL
					&& rec->getLevelType()==LV_ABOV_GND && rec->getLevelValue()==2)
			|| (rec->getDataType()==GRB_HUMID_REL
					&& rec->getLevelType()==LV_ISOBARIC
					&& (   rec->getLevelValue()==1000
						|| rec->getLevelValue()==975
						|| rec->getLevelValue()==950
						|| rec->getLevelValue()==925
						|| rec->getLevelValue()==900
						|| rec->getLevelValue()==875
						|| rec->getLevelValue()==850
						|| rec->getLevelValue()==825
						|| rec->getLevelValue()==800
						|| rec->getLevelValue()==775
						|| rec->getLevelValue()==750
						|| rec->getLevelValue()==725
						|| rec->getLevelValue()==700
						|| rec->getLevelValue()==650
						|| rec->getLevelValue()==600
						|| rec->getLevelValue()==550
						|| rec->getLevelValue()==500
						|| rec->getLevelValue()==400
						|| rec->getLevelValue()==300 
						|| rec->getLevelValue()==200 
						|| rec->getLevelValue()==100 ) )
			//-----------------------------------------
			|| (rec->getDataType()==GRB_TEMP_POT
					&& rec->getLevelType()==LV_SIGMA && rec->getLevelValue()==9950)
			//-----------------------------------------
            || (rec->getDataType()==GRB_CAPE
                            && rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
            || (rec->getDataType()==GRB_CAPE
                            && rec->getLevelValue()==LV_PRIVATE && rec->getLevelValue()==0)
            || (rec->getDataType()==GRB_CIN
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
			|| (rec->getDataType()==GRB_WIND_GUST
					&& rec->getLevelType()==LV_GND_SURF && rec->getLevelValue()==0)
            // added by david
//            || (rec->getDataType()==GRB_WIND_GUST
//                    && rec->getLevelType()==LV_ABOV_GND && rec->getLevelValue()==10)
            || (rec->getDataType()==GRB_COMP_REFL
                    && rec->getLevelType()==LV_ATMOS_ALL)
            //-----------------------------------------
			|| (rec->getLevelType()==LV_GND_SURF
				&& (   rec->getDataType()==GRB_WAV_SIG_HT
					|| rec->getDataType()==GRB_WAV_DIR
					|| rec->getDataType()==GRB_WAV_PER
					|| rec->getDataType()==GRB_WAV_WND_DIR
					|| rec->getDataType()==GRB_WAV_WND_HT
					|| rec->getDataType()==GRB_WAV_WND_PER
					|| rec->getDataType()==GRB_WAV_SWL_DIR
					|| rec->getDataType()==GRB_WAV_SWL_HT
					|| rec->getDataType()==GRB_WAV_SWL_PER
					|| rec->getDataType()==GRB_WAV_PRIM_DIR
					|| rec->getDataType()==GRB_WAV_PRIM_PER
					|| rec->getDataType()==GRB_WAV_SCDY_DIR
					|| rec->getDataType()==GRB_WAV_SCDY_PER
					|| rec->getDataType()==GRB_WAV_WHITCAP_PROB
					|| rec->getDataType()==GRB_WAV_MAX_PER
					|| rec->getDataType()==GRB_WAV_MAX_DIR
					|| rec->getDataType()==GRB_WAV_MAX_HT
				))
			)
			{
				storeRecordInMap (rec);
				rec = nullptr; // release ownership
				ok = true;   // at least 1 record ok
			}
			else {
				fprintf(stderr,
					"GribReader: id=%d unknown data: key=0x%lx  idCenter==%d && idModel==%d && idGrid==%d dataType==%d\n",
					rec->getId(),
					rec->getKey(),
					rec->getIdCenter(), rec->getIdModel(), rec->getIdGrid(),
					rec->getDataType()
					);
			}
		}
    } while (continueDownload && !eof);

    delete rec;
	if (! continueDownload)
		ok = false;
}
//---------------------------------------------------------------------------------
void  GribReader::computeAccumulationRecords (DataCode dtc)
{
    std::set<time_t>  setdates = getListDates();
    GribRecord *prev = nullptr;
    int p1 = 0, p2 = 0;

    if (setdates.empty())
        return;

	// XXX only work if P2 -P1 === delta time
    std::set<time_t>::reverse_iterator rit;
	for (rit = setdates.rbegin(); rit != setdates.rend(); ++rit)
    {
		time_t date = *rit;
		GribRecord *rec = getRecord( dtc, date );
		if ( !rec )
			continue;

		// XXX double check reference date and timerange 
		if (prev != nullptr ) {
			if (prev->getPeriodP1() == rec->getPeriodP1()) {
				// printf("substract %d %d %d\n", prev->getPeriodP1(), prev->getPeriodP2(), prev->getPeriodSec());
				if (rec->getTimeRange() == 4) {
					// accumulation 
					// prev = prev -rec
					prev->substract(*rec);
					p1 = rec->getPeriodP2();
				}
				else if (rec->getTimeRange() == 3) {
					// average
					// prev = (prev*d2 - rec*d1) / (double) (d2 - d1);
					prev->average(*rec);
					p1 = rec->getPeriodP2();
				}
			}
			if (p2 > p1 && rec->getTimeRange() == 4) {
				prev->multiplyAllData( 1.0/(p2 -p1) );
			}
		}
		prev = rec;
        p1 = prev->getPeriodP1();
		p2 = prev->getPeriodP2();
	}
	if (prev != nullptr && p2 > p1 && prev->getTimeRange() == 4 ) {
	    // the last one
        prev->multiplyAllData( 1.0/(p2 -p1) );
	}
}

//---------------------------------------------------------------------------------
void  GribReader::computeAccumulationRecords ()
{
	computeAccumulationRecords (DataCode(GRB_PRECIP_TOT,  LV_GND_SURF, 0));
	computeAccumulationRecords (DataCode(GRB_PRECIP_RATE, LV_GND_SURF, 0));
	computeAccumulationRecords (DataCode(GRB_CLOUD_TOT,   LV_ATMOS_ALL, 0));
}

//---------------------------------------------------------------------------------
void  GribReader::copyFirstCumulativeRecord ()
{
	copyFirstCumulativeRecord (DataCode(GRB_TMIN, LV_ABOV_GND, 2));
	copyFirstCumulativeRecord (DataCode(GRB_TMAX, LV_ABOV_GND, 2));
    copyFirstCumulativeRecord (DataCode(GRB_CLOUD_TOT,   LV_ATMOS_ALL, 0));
	copyFirstCumulativeRecord (DataCode(GRB_PRECIP_TOT,  LV_GND_SURF, 0));
	copyFirstCumulativeRecord (DataCode(GRB_PRECIP_RATE, LV_GND_SURF, 0));
	copyFirstCumulativeRecord (DataCode(GRB_SNOW_CATEG,  LV_GND_SURF, 0));
	copyFirstCumulativeRecord (DataCode(GRB_FRZRAIN_CATEG, LV_GND_SURF, 0));
}
//---------------------------------------------------------------------------------
void  GribReader::removeFirstCumulativeRecord ()
{
	removeFirstCumulativeRecord (DataCode(GRB_TMIN, LV_ABOV_GND, 2));
	removeFirstCumulativeRecord (DataCode(GRB_TMAX, LV_ABOV_GND, 2));
    removeFirstCumulativeRecord (DataCode(GRB_CLOUD_TOT,   LV_ATMOS_ALL, 0));
	removeFirstCumulativeRecord (DataCode(GRB_PRECIP_TOT,  LV_GND_SURF, 0));
	removeFirstCumulativeRecord (DataCode(GRB_PRECIP_RATE, LV_GND_SURF, 0));
	removeFirstCumulativeRecord (DataCode(GRB_SNOW_CATEG,  LV_GND_SURF, 0));
	removeFirstCumulativeRecord (DataCode(GRB_FRZRAIN_CATEG, LV_GND_SURF, 0));
}
//---------------------------------------------------------------------------------
void  GribReader::removeFirstCumulativeRecord (DataCode dtc)
{
	time_t dateref = getRefDateForData (dtc);
	GribRecord *rec = getFirstGribRecord (dtc);

	if (rec!= nullptr && rec->getRecordCurrentDate() == dateref)
	{
		std::vector<GribRecord *> *liste = getListOfGribRecords (dtc);
		if (liste != nullptr) {
			std::vector<GribRecord *>::iterator it;
			for (it=liste->begin(); it!=liste->end() && (*it)!=rec; ++it)
			{
			}
			assert(it!=liste->end());
			if ((*it) == rec) {
				liste->erase(it);
			}
		}
	}
}
//---------------------------------------------------------------------------------
void  GribReader::copyFirstCumulativeRecord (DataCode dtc)
{
	time_t dateref = getRefDateForData (dtc);
	GribRecord *rec = getRecord (dtc, dateref);
	if (rec == nullptr && dateref != 0)
	{
		rec = getFirstGribRecord (dtc);
		if (rec != nullptr)
		{
			GribRecord *r2 = new GribRecord (*rec, false);
			r2->setRecordCurrentDate (dateref);    // 1er enregistrement factice
			storeRecordInMap (r2);
		}
	}
}
//---------------------------------------------------------------------------------
void  GribReader::copyMissingWaveRecords (DataCode dtc)
{
	std::set<time_t>  setdates = getListDates();
	std::set<time_t>::iterator itd, itd2;
	for (itd=setdates.begin(); itd!=setdates.end(); ++itd) {
		time_t date = *itd;
		GribRecord *rec = getRecord (dtc, date);
		if (rec != nullptr)
			continue;
		itd2 = itd;
		do {
			++itd2;	// next date
			if (itd2 == setdates.end())
				break;
			time_t date2 = *itd2;
			GribRecord *rec2 = getRecord (dtc, date2);
			if (rec2) {
				if (!rec2->isDuplicated()) {
					// create a copied record from date2
					GribRecord *r2 = new GribRecord (*rec2, false);
					r2->setRecordCurrentDate (date);
					storeRecordInMap (r2);
				}
				break;
			}
		} while (1);
	}
}
//---------------------------------------------------------------------------------
void  GribReader::copyMissingWaveRecords ()
{
	copyMissingWaveRecords (DataCode(GRB_WAV_SIG_HT,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_DIR,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_PER,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_WND_DIR,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_WND_HT,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_WND_PER,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_SWL_DIR,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_SWL_HT,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_SWL_PER,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_PRIM_DIR,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_PRIM_PER,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_SCDY_DIR,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_SCDY_PER,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_WHITCAP_PROB,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_MAX_DIR,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_MAX_PER,LV_GND_SURF,0));
	copyMissingWaveRecords (DataCode(GRB_WAV_MAX_HT,LV_GND_SURF,0));
}
//---------------------------------------------------------------------------------
void  GribReader::removeMissingWaveRecords ()
{
	std::vector<GribRecord *>::iterator itv;
	for (auto const & it: mapGribRecords) {
		std::vector<GribRecord *> *ls = it.second;
		for (itv=ls->begin(); itv!=ls->end();  ) {
			GribRecord *rec = *itv;
			if (rec && rec->isOk()
				  && rec->isWaveData() && rec->isDuplicated()) 
			{
				delete rec;
				itv = ls->erase (itv);
			}
			else {
				++itv;
			}
		}
	}
}

//---------------------------------------------------------------------------------
void GribReader::readGribFileContent (int nbrecs)
{
    fileSize = zu_filesize(file);
	
    readAllGribRecords (nbrecs);
    createListDates ();
	computeMissingData ();   // RH DewPoint ThetaE
}
//----------------------------------------------------------------------------
void GribReader::computeMissingData ()
{
	//-----------------------------------------------------
	// Convert specific humidity to relative humidity
	//-----------------------------------------------------
	if (   getNumberOfGribRecords (DataCode(GRB_HUMID_REL, LV_ABOV_GND, 2)) == 0
	    && getNumberOfGribRecords (DataCode(GRB_HUMID_SPEC, LV_ABOV_GND, 2)) > 0)
	{
		
		for (long date : setAllDates)
		{
            GribRecord *recModel = getRecord (DataCode(GRB_HUMID_SPEC,LV_ABOV_GND,2),date);
			if (recModel != nullptr)
			{
				GribRecord *recHumidRel = new GribRecord(*recModel);

                recHumidRel->setDataType (GRB_HUMID_REL);
                for (int i=0; i<recModel->getNi(); i++)
                {
                    for (int j=0; j<recModel->getNj(); j++)
                    {
                        double x, y;
                        recModel->getXY(i,j, &x, &y);
                        double dp = computeHumidRel (x, y, date);
                        recHumidRel->setValue(i, j, dp);
                    }
                }
                storeRecordInMap (recHumidRel);
            }
		}
	}
	
	//-----------------------------------------------------
	// Are dewpoint data in file ?
	// If no, compute it with Magnus-Tetens formula, if possible.
	//-----------------------------------------------------
	dewpointDataStatus = DATA_IN_FILE;
	if (getNumberOfGribRecords (DataCode(GRB_DEWPOINT, LV_ABOV_GND, 2)) == 0)
	{
		dewpointDataStatus = NO_DATA_IN_FILE;
		if (  getNumberOfGribRecords (DataCode(GRB_HUMID_REL, LV_ABOV_GND, 2)) > 0
		   && getNumberOfGribRecords (DataCode(GRB_TEMP, LV_ABOV_GND, 2)) > 0)
		{
			dewpointDataStatus = COMPUTED_DATA;
			for (auto date : setAllDates)
			{
                GribRecord *recTemp = getRecord (DataCode(GRB_TEMP,LV_ABOV_GND,2),date);
                GribRecord *recHumid = getRecord (DataCode(GRB_HUMID_REL,LV_ABOV_GND,2), date);
                if (recTemp != nullptr && recHumid != nullptr)
				{
					// Cr??e un GribRecord avec les dewpoints calcul??s
					GribRecord *recModel = recTemp;
					GribRecord *recDewpoint = new GribRecord(*recModel);
                    recDewpoint->setDataType (GRB_DEWPOINT);
                    for (int i=0; i<recModel->getNi(); i++)
                    {
                        for (int j=0; j<recModel->getNj(); j++)
                        {
                            double x,y;

                            recModel->getXY(i,j, &x, &y);
                            double temp = recTemp->getInterpolatedValue   (x, y);
                            double humid = recHumid->getInterpolatedValue (x, y);
                            double dp = DataRecordAbstract::dewpointHardy (temp, humid);
                            recDewpoint->setValue(i, j, dp);
                        }
                    }
                    storeRecordInMap (recDewpoint);
                }
			}
		}
	}
	//-----------------------------------------------------
	// Precompute theta-e records in altitude
	//-----------------------------------------------------
	if (hasAltitude)
	{
		double thmin = 10000;
		double thmax = -10000;
		std::set<Altitude> allAlts = getAllAltitudes (GRB_HUMID_REL);
		for (auto altitude : allAlts)
		{	// all altitudes
				//DBGQS(AltitudeStr::toString(altitude));
			for (long date : setAllDates)
			{	// all dates
					//GribRecord *recHumidSpec = getRecord (DataCode(GRB_HUMID_SPEC,altitude.levelType, altitude.levelValue),date);
				GribRecord *recHumidRel = getRecord (DataCode(GRB_HUMID_REL,altitude.levelType, altitude.levelValue),date);
				GribRecord *recTemp = getRecord (DataCode(GRB_TEMP,altitude.levelType, altitude.levelValue),date);
				// if (recHumidSpec && recTemp)
				if (recHumidRel && recTemp)
				{  // Cr??e un GribRecord avec les theta-e calcul??es
					GribRecord *recThetaE = new GribRecord (*recTemp);
                    recThetaE->setDuplicated (false);
                    recThetaE->setDataType (GRB_PRV_THETA_E);
                    double P = -1;
                    if (altitude.levelType == LV_ISOBARIC)
                        P = altitude.levelValue;
                    else if (altitude.levelType == LV_ABOV_GND)
                        P = Therm::m2hpa (altitude.levelValue);
                    if (P > 0) {
                        for (int i=0; i<recThetaE->getNi(); i++)
                        {
                            for (int j=0; j<recThetaE->getNj(); j++)
                            {
                                double T = recTemp->getValue (i, j);
                                double RH = recHumidRel->getValue (i, j);
                                double thetae = Therm::thetaEfromHR (T,P,RH);
                                recThetaE->setValue (i, j, thetae);
                                if (thetae > thmax) thmax=thetae;
                                if (thetae < thmin) thmin=thetae;
                            }
                        }
                        storeRecordInMap (recThetaE);
                    }
                }
			}
		}
		//DBG("%g %g", thmin-273.15,thmax-273.15);
	}
}
//-------------------------------------------------------
double GribReader::computeDewPoint (double lon, double lat, time_t now)
{
	double dewpoint = GRIB_NOTDEF;

	GribRecord *recTempDew =  getRecord (DataCode(GRB_DEWPOINT,LV_ABOV_GND,2), now);
	if (recTempDew != nullptr)
	{
		// GRIB file contains dew point data
		dewpoint = recTempDew->getInterpolatedValue(lon, lat);
	}
	else
	{
		// Compute dew point with Magnus-Tetens formula
		GribRecord *recTemp =  getRecord (DataCode(GRB_TEMP,LV_ABOV_GND,2), now);
		GribRecord *recHumid = getRecord (DataCode(GRB_HUMID_REL,LV_ABOV_GND,2), now);
		if (recTemp && recHumid) {
			double temp = recTemp->getInterpolatedValue   (lon, lat);
			double humid = recHumid->getInterpolatedValue (lon, lat);
			dewpoint = DataRecordAbstract::dewpointHardy (temp, humid);
		}
	}
	return dewpoint;
}
//-------------------------------------------------------
double GribReader::computeHumidRel (double lon, double lat, time_t now)
{
	double hr = GRIB_NOTDEF;
	GribRecord *recHR =  getRecord (DataCode(GRB_HUMID_REL,LV_ABOV_GND,2), now);
	if (recHR != nullptr)
	{  // GRIB file contains HR
		hr = recHR->getInterpolatedValue(lon, lat);
	}
	else
	{  // Compute HR from HS
		GribRecord *recTemp =  getRecord (DataCode(GRB_TEMP,LV_ABOV_GND,2), now);
		GribRecord *recHS = getRecord (DataCode(GRB_HUMID_SPEC,LV_ABOV_GND,2), now);
		if (recTemp && recHS) {
			double tempK = recTemp->getInterpolatedValue   (lon, lat);
			double hs = recHS->getInterpolatedValue (lon, lat);
			hr = Therm::relHumidFromSpecific (tempK, hs);
		}
	}
	return hr;
}

//---------------------------------------------------
int GribReader::getDewpointDataStatus(int /*levelType*/,int /*levelValue*/)
{
	return dewpointDataStatus;
}

//---------------------------------------------------
int GribReader::getTotalNumberOfGribRecords() {
	int nb=0;
	for (auto const & it : mapGribRecords)
	{
		nb += it.second->size();
	}
	return nb;
}

//---------------------------------------------------
std::vector<GribRecord *> * GribReader::getFirstNonEmptyList()
{
    std::vector<GribRecord *> *ls = nullptr;
	for (auto it=mapGribRecords.begin(); ls==nullptr && it!=mapGribRecords.end(); ++it)
	{
		if (!(*it).second->empty())
			ls = (*it).second;
	}
	return ls;
}

//---------------------------------------------------
int GribReader::getNumberOfGribRecords (DataCode dtc)
{
	std::vector<GribRecord *> *liste = getListOfGribRecords (dtc);
	if (liste != nullptr)
		return liste->size();
	else
		return 0;
}

//---------------------------------------------------------------------
std::vector<GribRecord *> * GribReader::getListOfGribRecords (DataCode dtc)
{
	auto key = GribRecord::makeKey (dtc.dataType, dtc.levelType, dtc.levelValue);
	if (mapGribRecords.find(key) != mapGribRecords.end())
		return mapGribRecords[key];

    return nullptr;
}
//---------------------------------------------------------------------------
double  GribReader::getDateInterpolatedValue (
						DataCode dtc, double px, double py, time_t date)
{
	if (dtc.dataType == GRB_DEWPOINT) {
		if (dtc.levelType==LV_ABOV_GND && dtc.levelValue==2)
			return computeDewPoint(px, py, date);
	}
	else {
		GribRecord *rec = getRecord (dtc, date);
		if ( rec != nullptr)
			return rec->getInterpolatedValue (px, py);
	}
	return GRIB_NOTDEF;
}

//---------------------------------------------------------------------------
double  GribReader::get2DatesInterpolatedValue (
				DataCode dtc, double px, double py, time_t date)
{
	GribRecord *before, *after;
	findGribsAroundDate (dtc, date, &before, &after);
	return get2GribsInterpolatedValueByDate (px, py, date, before, after);
}
//------------------------------------------------------------------
void GribReader::findGribsAroundDate (DataCode dtc, time_t date,
							GribRecord **before, GribRecord **after)
{
	// Cherche les GribRecord qui encadrent la date
	std::vector<GribRecord *> *ls = getListOfGribRecords (dtc);
    *before = nullptr;
    *after  = nullptr;
	zuint nb = ls->size();
    for (zuint i=0; i<nb && *before==nullptr && *after==nullptr; i++)
	{
		GribRecord *rec = (*ls)[i];
		assert(rec->isOk());
		if (rec->getRecordCurrentDate() == date) {
			*before = rec;
			*after = rec;
		}
		else if (rec->getRecordCurrentDate() < date) {
			*before = rec;
		}
        else if (rec->getRecordCurrentDate() > date  &&  *before != nullptr) {
			*after = rec;
		}
	}
}
//------------------------------------------------------------------
double 	GribReader::get2GribsInterpolatedValueByDate (
				double px, double py, time_t date,
				GribRecord *before, GribRecord *after)
{
	double val = GRIB_NOTDEF;
    if (before!=nullptr && after!=nullptr) {
		if (before == after) {
			val = before->getInterpolatedValue(px, py);
		}
		else {
			time_t t1 = before->getRecordCurrentDate();
			time_t t2 = after->getRecordCurrentDate();
			if (t1 == t2) {
				val = before->getInterpolatedValue(px, py);
			}
			else {
				double v1 = before->getInterpolatedValue(px, py);
				double v2 = after->getInterpolatedValue(px, py);
				if (GribDataIsDef(v1) && GribDataIsDef(v2)) {
					double k  = fabs( (double)(date-t1)/(t2-t1) );
					val = (1.0-k)*v1 + k*v2;
				}
			}
		}
	}
	return val;
}

//---------------------------------------------------
// Rectangle of the area covered by the data
bool GribReader::getZoneExtension (double *x0,double *y0, double *x1,double *y1)
{
    if (ok) {
        *x0 = getXmin();
		*y0 = getYmin ();
        *x1 = getXmax();
		*y1 = getYmax ();
        return true;
    }

    return false;
}
//---------------------------------------------------
// Premier GribRecord trouv?? (pour r??cup??rer la grille)
GribRecord * GribReader::getFirstGribRecord()
{
    std::vector<GribRecord *> *ls = getFirstNonEmptyList();
    if (ls == nullptr)
    	return nullptr;

	assert(ls->at(0)->isOk());
	return ls->at(0);
}
//---------------------------------------------------
// Premier GribRecord (par date) pour un type donn??
GribRecord * GribReader::getFirstGribRecord (DataCode dtc)
{
	std::set<time_t>::iterator it;
	GribRecord *rec = nullptr;
	for (it=setAllDates.begin(); rec== nullptr && it!=setAllDates.end(); ++it)
	{
		time_t date = *it;
		rec = getRecord (dtc, date);
	}
	return rec;
}

//---------------------------------------------------
GribRecord * GribReader::getRecord (DataCode dtc, time_t date)
{
    std::vector<GribRecord *> *ls = getListOfGribRecords (dtc);
    GribRecord *res = nullptr;
    if (ls != nullptr) {
        // Cherche le premier enregistrement ?? la bonne date
        zuint nb = ls->size();
        for (zuint i=0; i<nb && res==nullptr; i++) {
            if ((*ls)[i]->isOk() && (*ls)[i]->getRecordCurrentDate() == date) {
                res = (*ls)[i];
			}
        }
    }
    return res;
}

//-------------------------------------------------------
// G??n??re la liste des dates pour lesquelles des pr??visions existent
void GribReader::createListDates()
{   // Le set assure l'ordre et l'unicit?? des dates
    setAllDates.clear();
	for (auto const & it : mapGribRecords )
	{
		std::vector<GribRecord *> *ls = it.second;
		for (auto & l : *ls) {
			assert(l->isOk());
			setAllDates.insert( l->getRecordCurrentDate() );
		}
	}
}


//-------------------------------------------------------------------------------
// Lecture compl??te d'un fichier GRIB
//-------------------------------------------------------------------------------
void GribReader::openFilePriv (const QString& fname, int nbrecs)
{
//     debug("Open file: %s", fname.c_str());
    fileName = fname;
    ok = false;
    clean_all_vectors();
    //--------------------------------------------------------
    // Ouverture du fichier
    //--------------------------------------------------------
    file = zu_open (qPrintable(fname), "rb", ZU_COMPRESS_AUTO);
    if (file == nullptr) {
        erreur("Can't open file: %s", qPrintable(fname));
        return;
    }
    
	emit newMessage (LongTaskMessage::LTASK_OPEN_FILE);
    if (nbrecs > 0) {
		emit newMessage (LongTaskMessage::LTASK_PREPARE_MAPS);
		readGribFileContent (nbrecs);
		// should be done once after opening all files.
		computeAccumulationRecords ();
	}
	else {
		ok = false;
	}
	zu_close (file);
}
//-------------------------------------------------------------------------------
// int GribReader::countGribRecords (ZUFILE *f, LongTaskProgress *taskProgress)
// {
// 	long fsize = zu_filesize(f);
// 	long i=0;
// 	int nb=0, j=0;
// 	char c;
// 	// count 'GRIB' patterns
// 	zu_rewind (f);
// 	while (taskProgress->continueDownload &&  zu_read (f, &c, 1) == 1) {
// 		i ++;
// 		if (i%1000 == 1)
// 			taskProgress->setValue ((int)(100.0*i/fsize));
// 		
// 		if (nb==0 && i>4) {
// 			break;	// not a grib file
// 		}
// 		
// 		if (  (j==0 && c=='G')
// 			||(j==1 && c=='R')
// 			||(j==2 && c=='I')
// 			||(j==3 && c=='B') )
// 			j ++;
// 		else
// 			j = 0;
// 		if (j == 4) {
// 			nb ++;
// 			j = 0;
// 		}
// 	}
// 	if (! taskProgress->continueDownload)
// 		nb = 0;
// 	zu_rewind (f);
// 	return nb;
// }
//-------------------------------------------------------------------------------
int GribReader::countGribRecords (ZUFILE *f)
{
	//qint64 fsize = zu_filesize(f);
	qint64 i=0;
	qint64 nb=0, j=0;
	char c;
	// count 'GRIB' patterns
	const int sizebuf = 300000;
	int nblus;
	char buf[sizebuf];
	zu_rewind (f);
	while (continueDownload && (nblus=zu_read(f,buf,sizebuf))>0) {
		for (i=0; i<nblus; i++) {
			c = buf[i];
			if (  (j==0 && c=='G')
				||(j==1 && c=='R')
				||(j==2 && c=='I')
				||(j==3 && c=='B') ) {
				j ++;
			}
			else if (j == 4) {
				nb ++;
				j = 0;
			}
			else {
				j = 0;
			}
		}
	}
	if (! continueDownload)
		nb = 0;
	zu_rewind (f);
	return nb;
}


//---------------------------------------------------------------------------------
time_t  GribReader::getRefDateForData (const DataCode &dtc)
{
	GribRecord *rec = getFirstGribRecord (dtc);
	if (rec)
		return rec->getRecordRefDate ();

    return 0;
}

//------------------------------------------------------------
time_t  GribReader::getFirstRefDate ()
{
	time_t t, t2;
	std::set<DataCode> all = getAllDataCode ();
	t = 0;
	for (auto dtc : all) {
			t2 = getRefDateForData (dtc); 
		if (t==0 || (t2!=0 && t>t2)) {
			t = t2;
		}
	}
	return t;
}
//------------------------------------------------------------
time_t  GribReader::getRefDateForDataCenter (const DataCenterModel &dcm)
{
	time_t t, t2;
	t = 0;
	for (auto const & it : mapGribRecords)
	{
		std::vector<GribRecord *> *ls = it.second;
		for (auto & l : *ls) {
			if (l->getDataCenterModel() == dcm) {
				t2 = l->getRecordRefDate (); 
				if (t==0 || (t2!=0 && t>t2)) {
					t = t2;
				}
			}
		}
		
	}
	return t;
}
