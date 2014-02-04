//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2010 Alessandro Tasora
// Copyright (c) 2013 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be 
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#ifndef CHLINKBASE_H
#define CHLINKBASE_H





#include "core/ChLog.h"
#include "physics/ChPhysicsItem.h"



namespace chrono
{




// Define the link type identifier.


#define LNK_BASE		29


///
/// Base class for all types of constraints that act like 
/// mechanical joints ('links').
///
///  Note that there are many specializations of this base class,
/// for example the ChLinkEngine class inherits this base class and
/// implements specific functions to represent an engine between two
/// bodies, etc. etc. (In fact, this base ChLink class does basically
/// _nothing_ unless it is specialized by some child class).
///

class ChApi ChLinkBase : public ChPhysicsItem 
{
	CH_RTTI(ChLinkBase,ChPhysicsItem);

protected:

				//
	  			// DATA
				//

	bool disabled;		// all constraints of link disabled because of user needs
	bool valid;			// link data is valid
	bool broken;		// link is broken because of excessive pulling/pushing.

public:
				//
	  			// CONSTRUCTORS
				//
	ChLinkBase ();
	virtual ~ChLinkBase ();
	virtual void Copy(ChLinkBase* source);  


public:
				//
	  			// FUNCTIONS
				//


				/// Tells if the link data is currently valid.
				/// (i.e. pointers to other items are correct)
	virtual bool IsValid() {return valid;}
				/// Set the status of link validity
	virtual void SetValid(bool mon) {valid = mon;}

				/// Tells if all constraints of this link are currently turned
				/// on or off by the user.
	virtual bool IsDisabled() {return disabled;}
				/// User can use this to enable/disable all the constraint of
				/// the link as desired.
	virtual void SetDisabled(bool mdis) {disabled = mdis;}


				/// Tells if the link is broken, for excess of pulling/pushing.
	virtual bool IsBroken() {return broken;}
				/// Ex:3rd party software can set the 'broken' status via this method
	virtual void SetBroken(bool mon) {broken = mon;}


				/// An important function!
				/// Tells if the link is currently active, in general,
				/// that is tells if it must be included into the system solver or not.
				/// This method cumulates the effect of various flags (so a link may
				/// be not active either because disabled, or broken, or not valid)
	virtual bool IsActive()
					{
						return ( valid &&
								!disabled &&
								!broken);
					}

				/// Get the type identifier of this link. Use if you don't want to use RTTI for performance.
	virtual int GetType	() {return LNK_BASE;}

				/// Get the number of scalar variables affected by constraints in this link 
	virtual int GetNumCoords() = 0;


				/// Get the link coordinate system, expressed relative to Body2 (the 'master'
				/// body). This represents the 'main' reference of the link: reaction forces 
				/// and reaction torques are expressed in this coordinate system.
				/// By default is in the origin of Body2, but child classes should implement this.
	virtual ChCoordsys<> GetLinkRelativeCoords() {return CSYSNORM;}

				/// Get the master coordinate system for the assets (should be implemented 
				/// by children classes)
	virtual ChFrame<> GetAssetsFrame(unsigned int nclone=0) { return ChFrame<>();}

				/// Tells if this link requires that the connected ChBody objects
				/// must be waken if they are sleeping. By default =true, i.e. always keep awaken, but
				/// child classes might return false for optimizing sleeping, in case no time-dependant.
	virtual bool IsRequiringWaking() {return true;}




			//
			// STREAMING
			//

					/// Method to allow deserializing a persistent binary archive (ex: a file)
					/// into transient data.
	virtual void StreamIN(ChStreamInBinary& mstream);

					/// Method to allow serializing transient data into a persistent
					/// binary archive (ex: a file).
	virtual void StreamOUT(ChStreamOutBinary& mstream);

					/// Method to allow serialization of transient data in ascii,
					/// as a readable item, for example   "chrono::GetLog() << myobject;"
	virtual void StreamOUT(ChStreamOutAscii& mstream) {};

};






} // END_OF_NAMESPACE____

#endif
