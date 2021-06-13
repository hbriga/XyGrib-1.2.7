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

#include <ctime>

#include "GribRecord.h"

//-------------------------------------------------------------------------------
// Adjust data type from different meteo center
// and identify the grib source
//-------------------------------------------------------------------------------
void  GribRecord::translateDataType ()
{
	this->knownData = true;
    if (dataType == GRB_PRECIP_RATE) {
     	// mm/s -> mm/h
        multiplyAllData( 3600.0 );
    }

	//------------------------
	// NOAA GFS
	//------------------------
	if (   idCenter==7
		&& (idModel==96 || idModel==81)		// NOAA
		&& (idGrid==0|| idGrid==3|| idGrid==4 || idGrid==255))	// Saildocs, Ocens
	{
		dataCenterModel = NOAA_GFS;
		// NOAA GFS product table differs from NCEP WW3 product table
		// data: http://nomads.ncdc.noaa.gov/data/gfsanl/
		if (idModel==81 && idGrid==3) {
			if (   dataType==GRB_WAV_MAX_DIR
				|| dataType==GRB_WAV_MAX_PER
				|| dataType==GRB_WAV_MAX_HT
			)
				dataType = GRB_TYPE_NOT_DEFINED;
		}
		// altitude level (entire atmosphere vs entire atmosphere considered as 1 level)
		if (levelType == LV_ATMOS_ENT) {
				levelType = LV_ATMOS_ALL;
		}
	}
	//------------------------
	// NOAA Waves
	//------------------------
	else if (   
			    (idCenter==7 && idModel==122 && idGrid==239)  // akw.all.grb
			 || (idCenter==7 && idModel==124 && idGrid==253)  // enp.all.grb
			 || (idCenter==7 && idModel==123 && idGrid==244)  // nah.all.grb
			 || (idCenter==7 && idModel==125 && idGrid==253)  // nph.all.grb
             || (idCenter==7 && idModel==88 && idGrid==233)   // nwww3.all.grb
             || (idCenter==7 && idModel==10 && idGrid==0)    // ww3.mean
             || (idCenter==7 && idModel==11 && idGrid==0)    // ww3 opengribs
             || (idCenter==7 && idModel==121 && idGrid==238)  // wna.all.grb
			 || (idCenter==7 && idModel==88 && idGrid==255)   // saildocs
	) {
		dataCenterModel = NOAA_NCEP_WW3;
	}
    //------------------------
    // NAM (CONUS, CACBN, PACIFIC)
    //------------------------
    else if (idCenter==7 && idModel==84) {
        dataCenterModel = NOAA_NAM;
    }
    //------------------------
    // GLOBAL GFS ENSEMBLE 
    // https://nomads.ncep.noaa.gov/txt_descriptions/GFS_Ensemble_high_resolution_doc.shtml
    //------------------------
    else if (idCenter==7 && idModel==107) {
        dataCenterModel = NOAA_GFS_ENSEMBLE;
    }
    //------------------------
    // Meteo France Arome/Arpege
    //------------------------
    else if (idCenter==84 && (idModel==204 || idModel==121 || idModel==211) && idGrid==255) {

		if ( (getDataType()==GRB_PRESSURE)
			&& getLevelType()==LV_MSL
			&& getLevelValue()==0) {
				dataType  = GRB_PRESSURE_MSL;
		}
		if ( (getDataType()==GRB_CLOUD_TOT)
			&& getLevelType()==LV_GND_SURF
			&& getLevelValue()==0) {
				levelType  = LV_ATMOS_ALL;
				levelValue = 0;
		}
        if (idModel==211) {
            if (Di>=0.5)
                dataCenterModel = MF_ARPEGE;
            else
                dataCenterModel = MF_ARPEGE_EU;
        }
        else if (idModel==204)
            dataCenterModel = MF_AROME;
	}
	else if ( idCenter==85 && idModel==26 ) {
		dataCenterModel = MF_WAM;
	}
//    else if (idCenter==84 && idModel==211 && idGrid==255){
//        dataCenterModel = MF_ARPEGE_GLOBAL;
//	}
	//------------------------										 
	// INMET   (resolution 0.025 - 2,8km or 0.00625 - 7km)				//HB
	//------------------------											//HB
	else if ( idCenter==43 )   {										//HB  
		    if (Di == 0.0250)  {										//HB  0,0250  - 2,8km
		       dataCenterModel = INMET28;								//HB
		    }                                                           //HB  
		    if (Di >= 0.0620)  {					   				    //HB  0,0620  - 7,0km
		       dataCenterModel = INMET70;								//HB
		    }                											//HB	
		    if (getDataType() ==  71) levelType = LV_ATMOS_ALL;			//HB  200	 (tcc)   	    
		    if (getDataType() == 145) dataType  = GRB_CAPE;				//HB  157
		    if (getDataType() == 146) dataType  = GRB_CIN;				//HB  156	
		    if (getDataType() == 230) dataType  = GRB_COMP_REFL;		//HB  212	    
		    if (getDataType() == 187) {									//HB
		    	dataType   = GRB_WIND_GUST;								//HB  180	
		    	levelType  = LV_GND_SURF;								//HB  001
		    }															//HB
	}																	//HB
	//------------------------										 
	// CPTEC															//HB
	//------------------------											//HB
	else if ( idCenter==46 )   {										//HB  
			dataCenterModel = CPTEC;									//HB	
	}																	//HB
	//------------------------
	// CEP navimail
	//------------------------
	else if ( idCenter==85 && idModel==1 && idGrid==255 ) {
		dataCenterModel = NORWAY_METNO;
	}
	//------------------------
	// http://retro.met.no/data/maritim/DNMI-NEurope.grb
	//------------------------
	else if (   
//			    (idCenter==88 && idModel==255 && idGrid==255)
//			 || (idCenter==88 && idModel==230 && idGrid==255)
//			 || (idCenter==88 && idModel==200 && idGrid==255)
//             || (idCenter==88 && idModel==67 && idGrid==255)
//             || (idCenter==88 && idModel==127 && idGrid==255)
             (idCenter==88 && idGrid==255)
    ) {
		dataCenterModel = NORWAY_METNO;
	}
	//----------------------------------------------
	// NCEP sea surface temperature
	//----------------------------------------------
	else if ((idCenter==7 && idModel==44 && idGrid==173)
	        || (idCenter==7 && idModel==44 && idGrid==235))
	{
		dataCenterModel = NOAA_NCEP_SST;
	}
	//----------------------------------------------
	// FNMOC WW3 mediterranean sea
	//----------------------------------------------
	else if (idCenter==58 && idModel==111 && idGrid==179)
	{
		dataCenterModel = FNMOC_WW3_MED;
	}
	//----------------------------------------------
	// FNMOC WW3 equatorial america and europa
	//----------------------------------------------
	else if (idCenter==58 && idModel==11 && idGrid==255)
	{
		dataCenterModel = FNMOC_WW3_EQAM;
	}
    //----------------------------------------------
    // SKIRON Model  http://openskiron.org/en
    //----------------------------------------------
    else if (idCenter==7 && idModel==31 && idGrid==255)
    {
        dataCenterModel = SKIRON;
    }
    //----------------------------------------------
    // DWD ICON Global and ICON-EU Nest
    //----------------------------------------------
    else if (idCenter==78 && idGrid==255 )
    {
        if ( (getDataType()==GRB_CLOUD_TOT)
            && getLevelType()==LV_GND_SURF
            && getLevelValue()==0) {
                levelType  = LV_ATMOS_ALL;
                levelValue = 0;
        }
        if ( (getDataType()==GRB_CAPE)
             && getLevelType()==LV_PRIVATE) {
            levelType = LV_GND_SURF;        // is actually ML multi-layer type data
        }
        if (idModel==1) dataCenterModel = DWD_ICON_GLOBAL;
        if (idModel==2) dataCenterModel = DWD_ICON_EU;


    }
    //----------------------------------------------
    // DWD EWAM
    //----------------------------------------------
    else if (idCenter==78 && idModel==202 && idGrid==255)
    {
        dataCenterModel = DWD_EWAM;
    }
    //----------------------------------------------
    // DWD GWAM
    //----------------------------------------------
    else if (idCenter==78 && idModel==199 && idGrid==255)
    {
        dataCenterModel = DWD_GWAM;
    }
    //----------------------------------------------
	// FNMOC WW3
	//----------------------------------------------
	else if (idCenter==58 && idModel==110 && idGrid==240)
	{
		dataCenterModel = FNMOC_WW3_GLB;
	}
    // ---------------------------------------------
    // COAMPS
    else if (idCenter==58 && idModel==13 && idGrid==255)
    {
        dataCenterModel = COAMPS;
    }

	//----------------------------------------------
	// Meteorem (Scannav)
	//----------------------------------------------
	else if (idCenter==59 && idModel==78 && idGrid==255)
	{
		if ( (getDataType()==GRB_WIND_VX || getDataType()==GRB_WIND_VY)
			&& getLevelType()==LV_MSL
			&& getLevelValue()==0)
		{
			levelType  = LV_ABOV_GND;
			levelValue = 0;
		}
		if ( getDataType()==GRB_PRECIP_TOT
			&& getLevelType()==LV_MSL
			&& getLevelValue()==0)
		{
			levelType  = LV_GND_SURF;
			levelValue = 0;
		}
	}
	//----------------------------------------------
	// NOGAPS
	//----------------------------------------------
	else if (idCenter==58 && idModel==58 && idGrid==240) // NOGAPS
	{
		dataCenterModel = NOGAPS;
		if (getDataType()==133) {
			DBG ("FIXME SEA TEMP");
			dataType = GRB_TEMP;
			levelType  = LV_GND_SURF;
			levelValue = 0;
		}
	}
	//----------------------------------------------
	// ECMWF ERA5
	//----------------------------------------------
    else if (idCenter==98 && (idModel==145|| idModel==255 ) && idGrid==255 && tableVersion == 128)
    {
        dataCenterModel = ECMWF_ERA5;
        if (getLevelType() == LV_ISOBARIC){ // for pressure level data
            if (getDataType() == 130)
            {
                dataType = GRB_TEMP;
            }
            else if (getDataType() == 131) // u wind
            {
                dataType = GRB_WIND_VX;
            }
            else if (getDataType() == 132)  // v wind
            {
                dataType = GRB_WIND_VY;
            }
            else if (getDataType() == 157)  // rh
            {
                dataType = GRB_HUMID_REL;
            }
            else if (getDataType() == 129)  // geopotential
            {
                dataType = GRB_GEOPOT_HGT;
                multiplyAllData(0.102);     // convert to geopot height
            }

        }
        if (getLevelType()==LV_GND_SURF && getLevelValue()==0) {  // single level data
            if (getDataType() == 141) // Snow depth  (m of water equivalent)
            {
                dataType = GRB_SNOW_DEPTH;
            }
            else if (getDataType() == 151)
            {
                dataType = GRB_PRESSURE_MSL;
                levelType = LV_MSL;
            }
            else if (getDataType() == 165 || getDataType() == 166)
            {
                if (getDataType() == 165)
                    dataType = GRB_WIND_VX;
                if (getDataType()== 166)
                    dataType = GRB_WIND_VY;
                levelType = LV_ABOV_GND;
                levelValue = 10;
            }
            else if (getDataType() == 167)
            {
                dataType = GRB_TEMP;
                levelType = LV_ABOV_GND;
                levelValue = 2;
            }
            else if (getDataType() == 168)
            {
                dataType = GRB_DEWPOINT;
                levelType = LV_ABOV_GND;
                levelValue = 2;
            }
            else if (getDataType() == 34)
            {
                dataType = -1; // Sea surface temperature (K)
            }
            else if (getDataType() == 164)
            {
                dataType = GRB_CLOUD_TOT;
                levelType = LV_ATMOS_ALL;
                multiplyAllData( 100.0 );
            }
            else if (getDataType() == 228)
            {
                dataType = GRB_PRECIP_TOT;
                // m/h -> mm/h
                multiplyAllData( 1000.0 );
            }
        }
    }

    else if (idCenter==227 && idModel==14 && idGrid==255 && tableVersion == 228)
    {
        dataCenterModel = NMC;
            switch (getDataType()) {
            case 41:
                dataType = GRB_PRESSURE_MSL;
                levelType = LV_MSL;
                break;
            case 105:
                dataType = GRB_HUMID_SPEC;
                break;
            case 132:
                dataType = GRB_PRECIP_TOT;
                // m/h -> mm/h
                multiplyAllData( 1000.0 );
                if (timeRange == 10) {
                    periodP2 = periodP1;
                    periodP1 = 0;
                    timeRange = 4;
                }
                break;
            case 192:
                dataType = GRB_WIND_VX;
                break;
            case 193:
                dataType = GRB_WIND_VY;
                break;
            case 98:
            case 194:
                dataType = GRB_TEMP;
                break;
            case 195:
                dataType = GRB_DEWPOINT;
                break;
            case 106:
            case 196:
                dataType = GRB_HUMID_REL;
                break;
            case 198:
                dataType = GRB_TMAX;
                break;
            case 199:
                dataType = GRB_TMIN;
                break;
            case 217:
                dataType = GRB_CLOUD_TOT;
                levelType = LV_ATMOS_ALL;
                multiplyAllData( 100.0 );
                break;
            case 240:
                dataType = GRB_CAPE;
                break;
            default:
                this->knownData = false;
            }
    }
    else if (idCenter==98 && idModel==145 && idGrid==255 && tableVersion == 228)
    {
        dataCenterModel = ECMWF_ERA5;
        if (getLevelType()==LV_GND_SURF && getLevelValue()==0) {
            if (getDataType() == 29)
            {
                dataType = GRB_WIND_GUST;
                // levelValue = 10; // XXX really 10 but we only display 0
            }
        }
    }
	//----------------------------------------------
	// ECMWF ERA5 WAVE
	//----------------------------------------------
    else if (idCenter==98 && idModel==111 && idGrid==255 && tableVersion == 140)
    {
        dataCenterModel = ECMWF_ERA5;
        switch (getDataType()) {
        case 229: // SWH Significant height of combined wind waves and swell (m)
            dataType = GRB_WAV_SIG_HT;
            break;
        case 230: // MWD Mean wave direction (Degree true)
            dataType = GRB_WAV_PRIM_DIR;
            break;
        case 232: // MWP Mean wave period (s)
            dataType = GRB_WAV_PRIM_PER;
            break;
        }
    }
	//------------------------------------------
    // PredictWind EMCWF grib1 Also ECMWF public data grib2
    // contributed by did-g
    //------------------------
    else if (idCenter==98 && (idModel==148 || idModel==149) && idGrid==255)
    {
        dataCenterModel = ECMWF;
        if (getDataType()==GRB_CLOUD_TOT
            && getLevelType()==LV_GND_SURF
            && getLevelValue()==0)
        {
            //dataType=71 levelType=1 levelValue=0
            levelType = LV_ATMOS_ALL;
        }
        else if (getDataType()==GRB_PRESSURE_MSL
            && getLevelType()==LV_GND_SURF
            && getLevelValue()==0)
        {
            // dataType=2 levelType=1 levelValue=0
            levelType = LV_MSL;
        }

        if (getDataType() == GRB_PRESSURE && getLevelType() == LV_MSL)
        {
            dataType = GRB_PRESSURE_MSL;
        }
    }
    else if (idCenter==98 && idModel==114 && idGrid==255)
    {
        dataCenterModel = ECMWF_WAVE;
    }
	//------------------------------------------
	// Others recognized grib suppliers
	//------------------------------------------
	else if (
		(idCenter==255 && idModel==1 && idGrid==255) //http://www.navcenter.com/grb/
	 || (idCenter==7   && idModel==96 && idGrid==3)    // Maxsea
	 || (idCenter==255 && idModel==255 && idGrid==255) // Maxsea: tide current
	 || (idCenter==7   && idModel==127 && idGrid==255) // Meteoconsult
	 || (idCenter==255 && idModel==220 && idGrid==255) // Actimar
	 || (idCenter==7   && idModel==45  && idGrid==255) // saildocs.com RTOFS GulfStream
	 || (idCenter==58 && idModel==22 && idGrid==179) // COAMPS
	 || (idCenter==58 && idModel==22 && idGrid==158) // COAMPS
     || (idCenter==58 && idModel==22 && idGrid==255) // COAMPS via Saildocs
     || (idCenter==54 && idModel==47 && idGrid==255) // Canada GEM
     || (idCenter==0 && idModel==0 && idGrid==255) // German Hydrographic Office
    ) {
	}
	//------------------------
	// Unknown center
	//------------------------
	else
	{
//        this->print("translateDataType() : Unknown Center GribRecord");
//		this->knownData = false;
        dataCenterModel = OTHER_DATA_CENTER;
        this->knownData = true;
    }
	
	//===================================================================
	if (this->knownData) {
		switch (getDataType()) {
			case GRB_WAV_SIG_HT:
			case GRB_WAV_DIR:
			case GRB_WAV_PER:
			case GRB_WAV_WND_DIR:
			case GRB_WAV_WND_HT:
			case GRB_WAV_WND_PER:
			case GRB_WAV_SWL_DIR:
			case GRB_WAV_SWL_HT:
			case GRB_WAV_SWL_PER:
			case GRB_WAV_PRIM_DIR:
			case GRB_WAV_PRIM_PER:
			case GRB_WAV_SCDY_DIR:
			case GRB_WAV_SCDY_PER:
			case GRB_WAV_WHITCAP_PROB:
			case GRB_WAV_MAX_DIR:
			case GRB_WAV_MAX_PER:
			case GRB_WAV_MAX_HT:
				waveData = true;
				levelType  = LV_GND_SURF;
				levelValue = 0;
				break;
			default:
				waveData = false;
		}
		if (getLevelType()== LV_GND_SURF) {
			levelValue = 0;
		}
		if (getLevelType()== LV_ABOV_GND && getLevelValue()== 0) {
			levelType  = LV_GND_SURF;
			levelValue = 0;
		}
	}
}

//----------------------------------------------------------------------------
// Try to detect ambiguous header from bad grib file providers :(
//----------------------------------------------------------------------------
bool GribRecord::verticalDataAreMirrored() {
	//---------------------------------------------
	// Malformed Maxsea grib file ?
	//---------------------------------------------
	if (    (idCenter==7 && idModel==96 && idGrid==4)   // same ident than NOAA
	     || (idCenter==7 && idModel==81 && idGrid==4)	// 2nd ident values
	     || (idCenter==7 && idModel==96 && idGrid==3)	// "oceanic" model
	     || (idCenter==7 && idModel==88 && idGrid==233)	// "oceanic" model
	     || (idCenter==255 && idModel==255 && idGrid==255)	// "oceanic" model: tide current
	) {
		if ( ! hasDiDj 
			&& savXmin < savXmax   
			&& savYmin > savYmax
			&& Di>0  && Dj>0
			&& isScanIpositive
			&& !isScanJpositive
		   )
		{
			verticalOrientationIsAmbiguous = true;
			return true;
		}
	}
	//---------------------------------------------
	// Malformed Meteorem (Scannav) grib file ?
	//---------------------------------------------
	else if (    (idCenter==59 && idModel==78 && idGrid==255) 
	) {
		if ( ! hasDiDj 
			&& savXmin < savXmax
			&& savYmin < savYmax
			&& Di>0  && Dj>0
			&& isScanIpositive
			&& !isScanJpositive
		   )
		{
			verticalOrientationIsAmbiguous = true;
			return true;
		}
	}
	else {
		//print ("verticalDataAreMirrored()");
	}
	
	return false;
}

//-------------------------------------------------------------------------------
void GribRecord::print (const char *title)
{
fprintf(stderr,"====== GribRecord %d : %s\n", id, title);
fprintf(stderr,"idCenter=%d idModel=%d idGrid=%d\n", idCenter,idModel,idGrid);
fprintf(stderr,"dataType=%d levelType=%d levelValue=%d\n", dataType, levelType,levelValue);
fprintf(stderr,"hour=%02g  cur=%s  ref=%s\n", (curDate-refDate)/3600.0,strCurDate,strRefDate);
fprintf(stderr,"sav   xmin=%f xmax=%f    ymin=%f ymax=%f   savDi=%f savDj=%f\n", savXmin,savXmax,savYmin,savYmax, savDi,savDj);
fprintf(stderr,"final xmin=%f xmax=%f    ymin=%f ymax=%f\n", xmin,xmax,ymin,ymax);
fprintf(stderr,"Ni=%d Nj=%d    entireWorldInLongitude=%d\n", Ni,Nj, (int)entireWorldInLongitude);
fprintf(stderr,"hasDiDj=%d Di,Dj=(%f %f)\n", hasDiDj, savDi,savDj);
fprintf(stderr,"final     Di,Dj=(%f %f)\n", Di,Dj);
fprintf(stderr,"hasBMS=%d isScanIpositive=%d isScanJpositive=%d isAdjacentI=%d\n",
                       hasBMS, isScanIpositive,isScanJpositive,isAdjacentI );
}

//-------------------------------------------------------------------------------
// Lecture depuis un fichier
//-------------------------------------------------------------------------------
GribRecord::GribRecord (ZUFILE* file, int id_)
{
    id = id_;
    seekStart = zu_tell(file);
    eof     = false;
	knownData = true;
	editionNumber = 0;
	setDuplicated (false);
	dataCenterModel = OTHER_DATA_CENTER;
    ok = readGribSection0_IS (file);
    if (ok) {
        ok = readGribSection1_PDS (file);
        ok = ok && zu_seek(file, fileOffset1+sectionSize1, SEEK_SET) == 0;
    }
    if (ok) {
        ok = readGribSection2_GDS (file);
        ok = ok && zu_seek(file, fileOffset2+sectionSize2, SEEK_SET) == 0;
    }
    if (ok) {
        ok = readGribSection3_BMS (file);
        ok = ok && zu_seek(file, fileOffset3+sectionSize3, SEEK_SET) == 0;
    }
    if (ok) {
        ok = readGribSection4_BDS (file);
        ok = ok && zu_seek(file, fileOffset4+sectionSize4, SEEK_SET) == 0;
    }
    if (ok) {
        ok = readGribSection5_ES (file);
    }
    
    if (ok) {
//        zu_seek (file, seekStart+totalSize, SEEK_SET);
    }
	
	if (ok && hasBMS) { // replace the BMS bits table with a faster bool table
        boolBMStab = new bool [Ni*Nj];
		assert (boolBMStab);
        for (int j=0; j<Nj; j++) {
		    for (int i=0; i<Ni; i++) {
				boolBMStab [j*Ni+i] = hasValueInBitBMS (i,j);
			}
		}
	}
	
	checkOrientation ();
    if (ok) {
		translateDataType ();
		setDataType (dataType);
		
		entireWorldInLongitude = (fabs(xmax-xmin)>=360.0)||(fabs(xmax-360.0+Di-xmin) < fabs(Di/20));
		//DBG("xmin=%g xmax=%g Di=%g %g entireWorldInLongitude=%d : ", xmin, xmax, Di, 
		//			fabs(xmax-360.0+Di-xmin),(int)entireWorldInLongitude);
		//this->print("");
	}
	else {
		//this->print("");
	}
}
//-------------------------------------------------------------------------------
// Constructeur de recopie
//-------------------------------------------------------------------------------
GribRecord::GribRecord (const GribRecord &rec, bool copy)
{
    *this = rec;
	setDuplicated (true);
    if (rec.data != nullptr && copy) {
        int size = rec.Ni*rec.Nj;
        auto ptr = new data_t[size];
        for (int i=0; i<size; i++) {
            ptr[i] = rec.data.get()[i];
        }
        this->data = std::shared_ptr<data_t>(ptr, std::default_delete<data_t[]>());
    }

    // recopie les champs de bits
    if (rec.BMSbits != nullptr) {
        int size = rec.sectionSize3-6;
        this->BMSbits = new zuchar[size];
		assert (this->BMSbits);
        for (int i=0; i<size; i++)
            this->BMSbits[i] = rec.BMSbits[i];
    }
    if (rec.boolBMStab != nullptr) {
        int size = rec.Ni*rec.Nj;
        this->boolBMStab = new bool[size];
		assert (this->boolBMStab);
        for (int i=0; i<size; i++)
            this->boolBMStab[i] = rec.boolBMStab[i];
    }
	checkOrientation ();
}
//--------------------------------------------------------------------------
GribRecord::~GribRecord()
{
    delete [] BMSbits;
    delete [] boolBMStab;
}
//------------------------------------------------------------------------------
void  GribRecord::checkOrientation ()
{
	if (!ok || !data || ymin==ymax
		|| Ni<=1 || Nj<=1
	) {
		ok = false;
		return;
	}
	if (xmin==xmax) {
		if (Di >= 0)
			xmin = xmin-360.0;
		else
			xmin = xmin+360.0;
		Di = 360.0/Ni;
	}
		double v;
	if (xmin > xmax) 
	{
		reverseData ('H');
		v=xmin;  xmin=xmax;  xmax=v;
		Di = fabs(Di);
	}
	if (ymin > ymax) 
	{
		//printf("GribRecord::checkOrientation (): ymin>ymax => must reverse data\n");		
		reverseData ('V');
		v=ymin;  ymin=ymax;  ymax=v;
		Dj = fabs(Dj);
	}
	
    while (xmin<=-180) {
        xmin += 360.0;
        xmax += 360.0;
    }
	
	if (verticalDataAreMirrored()) 
	{
		//printf("GribRecord::checkOrientation (): AMBIGUOUS FILE => must reverse data\n");		
		reverseData ('V');
		Dj = fabs(Dj);
	}
}
//------------------------------------------------------------------------------
void GribRecord::reverseData (char orientation) // orientation = 'H' or 'V'
{
	int i, j, i1, j1, i2, j2;
	data_t v;
	bool b;
	if (orientation == 'H') 
	{
		for (j=0; j<Nj; j++) {
			for (i1=0,i2=Ni-1;  i1<i2;  i1++,i2--) // Reverse line j
			{
				v = data.get() [j*Ni+i1];
				data.get() [j*Ni+i1] = data.get() [j*Ni+i2];
				data.get() [j*Ni+i2] = v;
				if (boolBMStab) {
					b = boolBMStab [j*Ni+i1];
					boolBMStab [j*Ni+i1] = boolBMStab [j*Ni+i2];
					boolBMStab [j*Ni+i2] = b;
				}
			}
		} 
	}
	else if (orientation == 'V') 
	{
		for (i=0; i<Ni; i++) {
			for (j1=0,j2=Nj-1;  j1<j2;  j1++,j2--) // Reverse row i
			{
				v = data.get() [j1*Ni+i];
				data.get() [j1*Ni+i] = data.get() [j2*Ni+i];
				data.get() [j2*Ni+i] = v;
				if (boolBMStab) {
					b = boolBMStab [j1*Ni+i];
					boolBMStab [j1*Ni+i] = boolBMStab [j2*Ni+i];
					boolBMStab [j2*Ni+i] = b;
				}
			}
		}
	}
}	
//------------------------------------------------------------------------------
void  GribRecord::setDataCode(const DataCode &dtc)
{
	dataType = dtc.dataType;
	levelType = dtc.levelType;
	levelValue = dtc.levelValue;
	dataKey = makeKey(dataType, levelType, levelValue);
}
//------------------------------------------------------------------------------
void  GribRecord::setDataType(const zuchar t)
{
	dataType = t;
	dataKey = makeKey(dataType, levelType, levelValue);
}
//------------------------------------------------------------------------------
uint64_t GribRecord::makeKey(int dataType,int levelType,int levelValue)
{
    return (((static_cast<uint64_t>(dataType) << 16) | static_cast<uint64_t>(levelType)) << 32) | levelValue;
}
//-------------------------------------------------------------------------------
void  GribRecord::multiplyAllData(double val)
{
    data_t k = val;
	for (int j=0; j<Nj; j++) {
		for (int i=0; i<Ni; i++)
		{
			if (hasValue(i,j)) {
				data.get()[j*Ni+i] *= k;
			}
		}
	}
}

//-------------------------------------------------------------------------------
void GribRecord::average(const GribRecord &rec)
{

    // for now only average records of same size
    // this : 6-12
    // rec  : 6-9
    // compute average 9-12
    //
    // this : 0-12
    // rec  : 0-11
    // compute average 11-12

    if (rec.data == nullptr || !rec.isOk())
        return;

    if (data == nullptr || !isOk())
        return;

    if (Ni != rec.Ni || Nj != rec.Nj)
        return;

    if (getPeriodP1() != rec.getPeriodP1())
        return;

    double d2 = getPeriodP2() - getPeriodP1();
    double d1 = rec.getPeriodP2() - rec.getPeriodP1();

    if (d2 <= d1)
        return;

    zuint size = Ni *Nj;
    double diff = d2 -d1;
    for (zuint i=0; i<size; i++) {
        if (! GribDataIsDef(rec.data.get()[i]))
           continue;
        if (! GribDataIsDef(data.get()[i]))
           continue;

        data.get()[i] = (data.get()[i]*d2 -rec.data.get()[i]*d1)/diff;
    }
}

//-------------------------------------------------------------------------------
void GribRecord::substract(const GribRecord &rec, bool pos)
{
    // for now only substract records of same size
    if (rec.data == nullptr || !rec.isOk())
        return;

    if (data == nullptr || !isOk())
        return;

    if (Ni != rec.Ni || Nj != rec.Nj)
        return;

    zuint size = Ni *Nj;
    for (zuint i=0; i<size; i++) {
        if (rec.data.get()[i] == GRIB_NOTDEF)
           continue;
        if (! GribDataIsDef(data.get()[i])) {
            data.get()[i] = -rec.data.get()[i];
            // XXX BMSbits
            if (boolBMStab) {
                boolBMStab [i] = true;
            }
        }
        else
            data.get()[i] -= rec.data.get()[i];
        if (pos && data.get()[i] < 0.) {
            // clamp data ...
            data.get()[i] = 0.;
        }
    }
}

//==============================================================
// Lecture des données
//==============================================================
//----------------------------------------------
// SECTION 0: THE INDICATOR SECTION (IS)
//----------------------------------------------
bool GribRecord::readGribSection0_IS (ZUFILE* file) {
    char strgrib[4];
    fileOffset0 = zu_tell(file);

// DBG ("START ftell=%ld %lx", zu_tell (file), zu_tell (file));
	// Cherche le 1er 'G' de 'GRIB'
	strgrib[0] = 'X';
	while ( (zu_read(file, strgrib, 1) == 1)
		   		&&  (strgrib[0] != 'G') )
	{ 
// 		fprintf(stderr,"%02X ", (unsigned char) strgrib[0]);
	}

    if (strgrib[0] != 'G') {
        ok = false;
        eof = true;
        return false;
    }
    if (zu_read(file, strgrib+1, 3) != 3) {
        ok = false;
        eof = true;
        return false;
    }
    if (strncmp(strgrib, "GRIB", 4) != 0)  {
        erreur("readGribSection0_IS(): Unknown file header : %c%c%c%c",
                    strgrib[0],strgrib[1],strgrib[2],strgrib[3]);
        ok = false;
        eof = true;
        return false;
    }
    totalSize = readInt3(file);

    editionNumber = readChar(file);
    if (editionNumber != 1)  {	// 1=GRIB1, 2=GRIB2
        ok = false;
        eof = true;
        return false;
    }

    return true;
} 
//----------------------------------------------
// SECTION 1: THE PRODUCT DEFINITION SECTION (PDS)
//----------------------------------------------
bool GribRecord::readGribSection1_PDS(ZUFILE* file) {
    fileOffset1 = zu_tell(file);
    if (zu_read(file, data1, 28) != 28) {
        ok=false;
        eof = true;
        return false;
    }
    sectionSize1 = makeInt3(data1[0],data1[1],data1[2]);
    tableVersion = data1[3];
    idCenter = data1[4];
    idModel  = data1[5];
    idGrid   = data1[6];
    hasGDS = (data1[7]&128)!=0;
    hasBMS = (data1[7]&64)!=0;

    dataType  = (int) data1[8];	 // octet 9 = parameters and units
	levelType = (int) data1[9];
	levelValue = (int) makeInt2(data1[10],data1[11]);

    refyear   = (data1[24]-1)*100+data1[12];
    refmonth  = data1[13];
    refday    = data1[14];
    refhour   = data1[15];
    refminute = data1[16];

    refDate = UTC_mktime(refyear,refmonth,refday,refhour,refminute,0);
	sprintf(strRefDate, "%04d-%02d-%02d %02d:%02d", refyear,refmonth,refday,refhour,refminute);

    periodP1  = data1[18];
    periodP2  = data1[19];
    timeRange = data1[20];
    periodsec = periodSeconds(data1[17],data1[18],data1[19],timeRange);
    curDate = UTC_mktime(refyear,refmonth,refday,refhour,refminute,periodsec);

    int decim;
    decim = (int)(((((zuint)data1[26]&0x7F)<<8)+(zuint)data1[27])&0x7FFF);
    if (data1[26]&0x80)
        decim *= -1;
    decimalFactorD = pow(10.0, decim);

    // Controls
    if (! hasGDS) {
        erreur("Record %d: GDS not found",id);
        ok = false;
    }
    if (decimalFactorD == 0) {
        erreur("Record %d: decimalFactorD null",id);
        ok = false;
    }
    return ok;
}
//----------------------------------------------
// SECTION 2: THE GRID DESCRIPTION SECTION (GDS)
//----------------------------------------------
bool GribRecord::readGribSection2_GDS(ZUFILE* file) {
    if (! hasGDS)
        return false;
    fileOffset2 = zu_tell(file);
    sectionSize2 = readInt3(file);  // byte 1-2-3
    NV = readChar(file);			// byte 4
    PV = readChar(file); 			// byte 5
    gridType = readChar(file); 		// byte 6

    if (gridType != 0     
               &&  gridType != 10						//HB     INMET sends this 
    		// && gridType != 4
		) {
        erreur("Record %d: unknown grid type GDS(6) : %d",id,gridType);
        ok = false;
    }

    Ni  = readInt2(file);				// byte 7-8
    Nj  = readInt2(file);				// byte 9-10
    ymin = readSignedInt3(file)/1000.0;	// byte 11-12-13
    xmin = readSignedInt3(file)/1000.0;	// byte 14-15-16
    resolFlags = readChar(file);		// byte 17
    ymax = readSignedInt3(file)/1000.0;	// byte 18-19-20
    xmax = readSignedInt3(file)/1000.0;	// byte 21-22-23
	savXmin = xmin;
	savXmax = xmax;
	savYmin = ymin;
	savYmax = ymax;

    Di  = readSignedInt2(file)/1000.0;	// byte 24-25
    Dj  = readSignedInt2(file)/1000.0;	// byte 26-27
	savDi = Di;
	savDj = Dj;
	
    while ( xmin> xmax   &&  Di >0) {   // horizontal size > 360 °
        xmin -= 360.0;
    }
     while ( xmax> 360) {
		xmin -= 360.0;
        xmax -= 360.0;
    }
    hasDiDj = (resolFlags&0x80) !=0;
    isEarthSpheric = (resolFlags&0x40) ==0;
    isUeastVnorth =  (resolFlags&0x08) ==0;

    scanFlags = readChar(file);			// byte 28
    isScanIpositive = (scanFlags&0x80) ==0;
    isScanJpositive = (scanFlags&0x40) !=0;
    isAdjacentI     = (scanFlags&0x20) ==0;
	
	if (Ni<=1 || Nj<=1) {
		erreur("Record %d: Ni=%d Nj=%d",id,Ni,Nj);
		ok = false;
	}
	else { 
		Di = (xmax-xmin) / (Ni-1);
		Dj = (ymax-ymin) / (Nj-1);
	}
// printf ("Ni=%d Nj=%d\n", Ni, Nj);
//print ("readGribSection2_GDS");	

    return ok;
}
//----------------------------------------------
// SECTION 3: BIT MAP SECTION (BMS)
//----------------------------------------------
bool GribRecord::readGribSection3_BMS(ZUFILE* file) {
    fileOffset3 = zu_tell(file);
    if (! hasBMS) {
        sectionSize3 = 0;
        return ok;
    }
    sectionSize3 = readInt3(file);
    (void) readChar(file);
    int bitMapFollows = readInt2(file);

    if (bitMapFollows != 0) {
        return ok;
    }
    if (sectionSize3 <= 6) {
        erreur("Record %d: Bad BMS size %d",id, sectionSize3);
        ok = false;
        return ok;
    }
    BMSbits = new zuchar[sectionSize3-6];
    if (!BMSbits) {
        erreur("Record %d: out of memory",id);
        ok = false;
    }
    for (zuint i=0; i<sectionSize3-6; i++) {
        BMSbits[i] = readChar(file);
    }
    return ok;
}
//----------------------------------------------
// SECTION 4: BINARY DATA SECTION (BDS)
//----------------------------------------------
bool GribRecord::readGribSection4_BDS(ZUFILE* file) {
    fileOffset4  = zu_tell(file);
    sectionSize4 = readInt3(file);  // byte 1-2-3

    zuchar flags  = readChar(file);			// byte 4
    scaleFactorE = readSignedInt2(file);	// byte 5-6
    refValue     = readFloat4(file);		// byte 7-8-9-10
    nbBitsInPack = readChar(file);			// byte 11
    scaleFactorEpow2 = pow(2,scaleFactorE);
    unusedBitsEndBDS = flags & 0x0F;
	
    isGridData      = (flags&0x80) == 0;
    isSimplePacking = (flags&0x40) == 0;
    isFloatValues   = (flags&0x20) == 0;
    hasAdditionalFlags = (flags&0x10) != 0;

// printf("BDS type=%3d - bits=%02d - level %3d - %d\n", dataType, nbBitsInPack, levelType,levelValue);

    if (! isGridData) {
        erreur("Record %d: need grid data",id);
        ok = false;
    }
    if (! isSimplePacking) {
        erreur("Record %d: need simple packing",id);
        ok = false;
    }
    if (! isFloatValues) {
        erreur("Record %d: need double values",id);
        ok = false;
    }
    if (hasAdditionalFlags) {
        erreur("Record %d: can't process additional flags",id);
        ok = false;
    }

    if (sectionSize4 < 11) {
        erreur("Record %d: can't process additional flags",id);
        ok = false;
    }

    if (!ok) {
        return ok;
    }

    // Allocate memory for the data
	auto ptr = new data_t[Ni*Nj];
    data = std::shared_ptr<data_t>(ptr, std::default_delete<data_t[]>());
    if (!data) {
        erreur("Record %d: out of memory",id);
        ok = false;
    }

    zuint  startbit  = 0;
    int  datasize = sectionSize4-11;
    zuchar *buf = new zuchar[datasize+4]();  // +4 pour simplifier les décalages ds readPackedBits
	
    if (!buf) {
        erreur("Record %d: out of memory",id);
        ok = false;
    }
    else if (zu_read(file, buf, datasize) != datasize) {
        erreur("Record %d: data read error",id);
        ok = false;
        eof = true;
    }
    if (!ok) {
        delete [] buf;
        return ok;
    }
    

    // Read data in the order given by isAdjacentI
    int i, j, x;
    int ind;
    if (isAdjacentI) {
        for (j=0; j<Nj; j++) {
            for (i=0; i<Ni; i++) {
                if (!hasDiDj && !isScanJpositive) {
                    ind = (Nj-1 -j)*Ni+i;
                }
                else {
                    ind = j*Ni+i;
                }
                if (hasValueInBitBMS(i,j)) {
                    x = readPackedBits(buf, startbit, nbBitsInPack);
                    data.get()[ind] = (refValue + x*scaleFactorEpow2)/decimalFactorD;
                    startbit += nbBitsInPack;
                }
                else {
                    data.get()[ind] = GRIB_NOTDEF;
                }
            }
        }
    }
    else {
        for (i=0; i<Ni; i++) {
            for (j=0; j<Nj; j++) {
                if (!hasDiDj && !isScanJpositive) {
                    ind = (Nj-1 -j)*Ni+i;
                }
                else {
                    ind = j*Ni+i;
                }
                if (hasValueInBitBMS(i,j)) {
                    x = readPackedBits(buf, startbit, nbBitsInPack);
                    startbit += nbBitsInPack;
                    data.get()[ind] = (refValue + x*scaleFactorEpow2)/decimalFactorD;
                }
                else {
                    data.get()[ind] = GRIB_NOTDEF;
                }
            }
        }
    }

    delete [] buf;
    return ok;
}



//----------------------------------------------
// SECTION 5: END SECTION (ES)
//----------------------------------------------
bool GribRecord::readGribSection5_ES (ZUFILE* file) {
	
// DBG ("7777? ftell=%ld %lx", zu_tell (file), zu_tell (file));

    char str[4];
    if (zu_read(file, str, 4) != 4) {
		DBG("can't read 4 chars");
        ok = false;
        eof = true;
        return false;
    }
    if (strncmp(str, "7777", 4) != 0)  {
        DBG ("Final 7777 not read: %c%c%c%c",str[0],str[1],str[2],str[3]);
        ok = false;
        return false;
    }
    return ok;
}






//==============================================================
// Fonctions utiles
//==============================================================
double GribRecord::readFloat4(ZUFILE* file) {
    unsigned char t[4];
    if (zu_read(file, t, 4) != 4) {
        ok = false;
        eof = true;
        return 0;
    }

    double val;
    int A = (zuint)t[0]&0x7F;
    int B = ((zuint)t[1]<<16)+((zuint)t[2]<<8)+(zuint)t[3];

    val = pow(2,-24)*B*pow(16,A-64);
    if (t[0]&0x80)
        return -val;
    else
        return val;
}
//----------------------------------------------
zuchar GribRecord::readChar(ZUFILE* file) {
    zuchar t;
    if (zu_read(file, &t, 1) != 1) {
        ok = false;
        eof = true;
        return 0;
    }
    return t;
}
//----------------------------------------------
int GribRecord::readSignedInt3(ZUFILE* file) {
    unsigned char t[3];
    if (zu_read(file, t, 3) != 3) {
        ok = false;
        eof = true;
        return 0;
    }
    int val = (((zuint)t[0]&0x7F)<<16)+((zuint)t[1]<<8)+(zuint)t[2];
    if (t[0]&0x80)
        return -val;
    else
        return val;
}
//----------------------------------------------
int GribRecord::readSignedInt2(ZUFILE* file) {
    unsigned char t[2];
    if (zu_read(file, t, 2) != 2) {
        ok = false;
        eof = true;
        return 0;
    }
    int val = (((zuint)t[0]&0x7F)<<8)+(zuint)t[1];
    if (t[0]&0x80)
        return -val;
    else
        return val;
}
//----------------------------------------------
zuint GribRecord::readInt3(ZUFILE* file) {
    unsigned char t[3];
    if (zu_read(file, t, 3) != 3) {
        ok = false;
        eof = true;
        return 0;
    }
    return ((zuint)t[0]<<16)+((zuint)t[1]<<8)+(zuint)t[2];
}
//----------------------------------------------
zuint GribRecord::readInt2(ZUFILE* file) {
    unsigned char t[2];
    if (zu_read(file, t, 2) != 2) {
        ok = false;
        eof = true;
        return 0;
    }
    return ((zuint)t[0]<<8)+(zuint)t[1];
}
//----------------------------------------------
zuint GribRecord::makeInt3(zuchar a, zuchar b, zuchar c) {
    return ((zuint)a<<16)+((zuint)b<<8)+(zuint)c;
}
//----------------------------------------------
zuint GribRecord::makeInt2(zuchar b, zuchar c) {
    return ((zuint)b<<8)+(zuint)c;
}
//----------------------------------------------
zuint GribRecord::readPackedBits(const zuchar *buf, zuint first, zuint nbBits)
{
    zuint oct = first / 8;
    zuint bit = first % 8;

    zuint val = (buf[oct]<<24) + (buf[oct+1]<<16) + (buf[oct+2]<<8) + (buf[oct+3]);
    val = val << bit;
    val = val >> (32-nbBits);
    return val;
}

//----------------------------------------------
void  GribRecord::setRecordCurrentDate (time_t t)
{
	curDate = t;
    struct tm *date = gmtime(&t);
    zuint year   = date->tm_year+1900;
    zuint month  = date->tm_mon+1;
	zuint day    = date->tm_mday;
	zuint hour   = date->tm_hour;
	zuint minute = date->tm_min;
	sprintf(strCurDate, "%04d-%02d-%02d %02d:%02d", year,month,day,hour,minute);
}
//----------------------------------------------
zuint GribRecord::periodSeconds(zuchar unit,zuchar P1,zuchar P2,zuchar range) {
    zuint res, dur;
    switch (unit) {
        case 0: //	Minute
            res = 60; break;
        case 1: //	Hour
            res = 3600; break;
        case 2: //	Day
            res = 86400; break;
        case 10: //	3 hours
            res = 10800; break;
        case 11: //	6 hours
            res = 21600; break;
        case 12: //	12 hours
            res = 43200; break;
        case 254: // Second
            res = 1; break;
        case 3: //	Month
        case 4: //	Year
        case 5: //	Decade (10 years)
        case 6: //	Normal (30 years)
        case 7: //	Century (100 years)
        default:
            erreur("id=%d: unknown time unit in PDS b18=%d",id,unit);
            res = 0;
            ok = false;
    }
    debug("id=%d: PDS (time range) b21=%d P1=%d P2=%d",id,range,P1,P2);
    dur = 0;
    switch (range) {
        case 0:
            dur = (zuint)P1; break;
        case 1:
            dur = 0; break;
        case 2:
        case 3:
            // dur = ((zuint)P1+(zuint)P2)/2; break;     // TODO
            dur = (zuint)P2; break;
         case 4: // Accumulation (reference time + P1 to reference time + P2) product considered valid at reference time + P2
            dur = (zuint)P2; break;
        case 10: // product valid at reference time + P1
            dur = ((zuint)P1<<8) + (zuint)P2; break;
        default:
            erreur("id=%d: unknown time range in PDS b21=%d",id,range);
            dur = 0;
            ok = false;
    }
    return res*dur;
}


//===============================================================================================
data_t GribRecord::getInterpolatedValue (double px, double py, bool interpolate) const
{
    double val; 
	double eps = 1e-4;
    double pi, pj;     // coord. in grid unit
    // 00 10      point is in a square
    // 01 11
    int i0, j0, i1, j1;
	double ddx, ddy;
	
    if (!ok || Di==0 || Dj==0) {
        return GRIB_NOTDEF;
    }
    if (!isYInMap(py)) {
		return GRIB_NOTDEF;
    } 
    if (!isXInMap(px)) {
		if (! entireWorldInLongitude) {
			px += 360.0;               // tour du monde à droite ?
			if (!isXInMap(px)) {
				px -= 2*360.0;              // tour du monde à gauche ?
				if (!isXInMap(px)) {
					return GRIB_NOTDEF;
				}
			}
			pi = (px-xmin)/Di;
			i0 = (int) floor(pi);  // point 00
			i1 = i0+1;
		}
		else {
			while (px< 0)
				px += 360;
			if (px <= xmax) {
				pi = (px-xmin)/Di;
				i0 = (int) floor(pi);  // point 00
				i1 = i0+1;
			}
			else {
				pi = (px-xmin)/Di;
				i0 = (int) floor(pi);  // point 00
				i1 = 0;
			}
		}
    } 
    else {
        if (px < xmin)
            px += 360.;
		pi = (px-xmin)/Di;
		i0 = (int) floor(pi);  // point 00
		i1 = i0+1;
	}
	pj = (py-ymin)/Dj;
	j0 = (int) floor(pj);
	j1 = j0+1;
	
	// value very close to a grid point ?
	ddx = fabs (pi-i0);
	ddy = fabs (pj-j0);
	int ii = (ddx<eps) ? i0 : ((1-ddx)<eps) ? i1 : -1;
	int jj = (ddy<eps) ? j0 : ((1-ddy)<eps) ? j1 : -1;
	if (ii>=0 && jj>=0) {
		if (hasValue(ii,jj))
			return getValue (ii, jj);
		else
			return GRIB_NOTDEF;
	}

    bool h00,h01,h10,h11;
    int nbval = 0;     // how many values in grid ?
    if ((h00=hasValue(i0, j0)))
        nbval ++;
    if ((h10=hasValue(i1, j0)))
        nbval ++;
    if ((h01=hasValue(i0, j1)))
        nbval ++;
    if ((h11=hasValue(i1, j1)))
        nbval ++;

    if (nbval <3) {
        return GRIB_NOTDEF;
    }

    // distances to 00
    double dx = pi-i0;
    double dy = pj-j0;

	if (! interpolate)
	{
		if (dx < 0.5) {
			if (dy < 0.5)
				val = getValue(i0, j0);
			else
				val = getValue(i0, j1);
		}
		else {
			if (dy < 0.5)
				val = getValue(i1, j0);
			else
				val = getValue(i1, j1);
		}
		return val;
	}

    dx = (3.0 - 2.0*dx)*dx*dx;   // pseudo hermite interpolation
    dy = (3.0 - 2.0*dy)*dy*dy;


    double xa, xb, xc, kx, ky;
    // Triangle :
    //   xa  xb
    //   xc
    // kx = distance(xa,x)
    // ky = distance(xa,y)
    if (nbval == 4)
    { 
        double x00 = getValue(i0, j0);
        double x01 = getValue(i0, j1);
        double x10 = getValue(i1, j0);
        double x11 = getValue(i1, j1);
        double x1 = (1.0-dx)*x00 + dx*x10;
        double x2 = (1.0-dx)*x01 + dx*x11;
        val =  (1.0-dy)*x1 + dy*x2;
        return val;
    }
    else {
        // here nbval==3, check the corner without data
        if (!h00) {
            //printf("! h00  %f %f\n", dx,dy);
            xa = getValue(i1, j1);   // A = point 11
            xb = getValue(i0, j1);   // B = point 01
            xc = getValue(i1, j0);   // C = point 10
            kx = 1-dx;
            ky = 1-dy;
        }
        else if (!h01) {
            //printf("! h01  %f %f\n", dx,dy);
            xa = getValue(i1, j0);   // A = point 10
            xb = getValue(i1, j1);   // B = point 11
            xc = getValue(i0, j0);   // C = point 00
            kx = dy;
            ky = 1-dx;
        }
        else if (!h10) {
            //printf("! h10  %f %f\n", dx,dy);
            xa = getValue(i0, j1);     // A = point 01
            xb = getValue(i0, j0);     // B = point 00
            xc = getValue(i1, j1);     // C = point 11
            kx = 1-dy;
            ky = dx;
        }
        else {
            //printf("! h11  %f %f\n", dx,dy);
            xa = getValue(i0, j0);  // A = point 00
            xb = getValue(i1, j0);  // B = point 10
            xc = getValue(i0, j1);  // C = point 01
            kx = dx;
            ky = dy;
        }
    }
    double k = kx + ky;
    if (k<0 || k>1) {
        val = GRIB_NOTDEF;
    }
    else if (k == 0) {
        val = xa;
    }
    else {
        // axes interpolation
        double vx = k*xb + (1-k)*xa;
        double vy = k*xc + (1-k)*xa;
        // diagonal interpolation
        double k2 = kx / k;
        val =  k2*vx + (1-k2)*vy;
    }
    return val;
}
//--------------------------------------------------------------------------
data_t GribRecord::getValueOnRegularGrid (DataCode dtc, int i, int j ) const
{
	if ( getDataCode() != dtc )
		return GRIB_NOTDEF;
    return getValue (i,j);
}
//--------------------------------------------------------------------------
data_t  GribRecord::getInterpolatedValue (
						DataCode dtc,
						double px, double py,
						bool interpolate) const 
{
	if ( getDataCode() != dtc )
		return GRIB_NOTDEF;
    return getInterpolatedValueUsingRegularGrid (dtc,px,py,interpolate);
}





