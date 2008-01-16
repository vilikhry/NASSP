#ifndef __checklistController_h
#define __checklistController_h
// Disable 4018 warning for the extent of this header file.
#pragma warning ( push )
#pragma warning ( disable:4018 )

#include "orbiterapi.h"
#include <vector>
#include <deque>
#include <string>
#include "orbiterSDK.h"
#include "nasspdefs.h"
#include "connector.h"
#include "BasicExcelVC6.hpp"
using namespace std;
using namespace YExcel;

#define ChecklistControllerStartString "<checklist>"
#define ChecklistControllerEndString "</checklist>"

/// -------------------------------------------------------------
/// Listing of available events to spawn a checklist off of.
/// -------------------------------------------------------------
enum RelativeEvent
{
	CHECKLIST_RELATIVE = -2, /// < check item time is relative to the beginning of the checklist.  WARNING! Not valid for groups.
	MISSION_TIME = -1, /// < check item happens at specific MJD WARNING! could cause unexpected results if items after this become scheduled to operate before it based on a checklist event starting late.  Should primarily only be used for primary checklists!
	NO_TIME_DEF = 0, /// < check item happens when it is reached NOTE: in auto mode, this item will inherit the state of last time-relative checklist item before it.  Groups with this definition will NOT be automated at all.
};

/// -------------------------------------------------------------
/// A "checklist group" which defines an available checklist
/// "program"
/// -------------------------------------------------------------
struct ChecklistGroup
{
/// -------------------------------------------------------------
/// Default constructor.
/// -------------------------------------------------------------
	ChecklistGroup()
	{
		group = -1;
		time = 0;
		relativeEvent = NO_TIME_DEF;
		manualSelect = false;
		autoSelect = false;
		essential = false;
		name[0] = 0;
	}
/// -------------------------------------------------------------
/// index defined at runtime.  Use this in a ChecklistItem struct
/// to start the checklist operation of that group.
/// -------------------------------------------------------------
	int group;
/// -------------------------------------------------------------
/// Time at which this group should be executed. (available at 
/// this time)
/// -------------------------------------------------------------
	double time;
/// -------------------------------------------------------------
/// Event relative to which the time triggers.
/// -------------------------------------------------------------
	RelativeEvent relativeEvent;
/// -------------------------------------------------------------
/// Group can be manually selected from the MFD.  All checklists
/// normally visible to the MFD have this set to true.
/// -------------------------------------------------------------
	bool manualSelect;
/// -------------------------------------------------------------
/// Checklist can be automatically selected.  If both this and
/// manualSelect are false, the checklist will never be displayed.
/// -------------------------------------------------------------
	bool autoSelect;
/// -------------------------------------------------------------
/// Checklist group must be executed and takes priority in auto
/// selection of checklist items over the existing item.
/// Should this be true, then at the defined time, it will begin
/// replacing the present checklist with the checklist group.
/// It will internally stack up the old group and return once the
/// group is done.
	bool essential;
/// -------------------------------------------------------------
/// Name of the group as should be displayed on the checklist.
/// -------------------------------------------------------------
	char name[100];
};
/// -------------------------------------------------------------
/// An individual element in a checklist program.  These elements
/// can be triggered as complete either by the user or the auto
/// complete code.  In quickstart mode, the element will be
/// switched automatically if enabled.
/// -------------------------------------------------------------
struct ChecklistItem
{
/// -------------------------------------------------------------
/// Default constructor, does a proper "empty" construction
/// -------------------------------------------------------------
	ChecklistItem()
	{
		group = -1;
		index = -1;
		time = 0;
		relativeEvent = NO_TIME_DEF;
		failEvent = -1;
		text[0] = 0;
		info[0] = 0;
		automatic = false;
		item[0] = 0;
		position = 0;
		complete = false;
		failed = false;
	}
/// -------------------------------------------------------------
/// index defined dynaimcally at runtime.  All available
/// checklists have a group id retrieved from the ChecklistGroup
/// structure.
/// -------------------------------------------------------------
	int group;
/// -------------------------------------------------------------
/// defined linearly.  This index always defines the item its 
/// same number of places from the beginning of the checklist 
/// group
/// -------------------------------------------------------------
	int index;
/// -------------------------------------------------------------
/// Either MET (if no relativeEvent is defined) or time before or
/// after the event.
/// -------------------------------------------------------------
	double time;
/// -------------------------------------------------------------
/// Event from the "events" enum listing the many managed events
/// of this system.
/// -------------------------------------------------------------
	RelativeEvent relativeEvent;
/// -------------------------------------------------------------
/// group id to go to in the event of a failure of this check step.
/// always null when received by the MFD.
/// -------------------------------------------------------------
	int failEvent;
/// -------------------------------------------------------------
/// Text to display describing the checklist.
/// -------------------------------------------------------------
	char text[100];
/// -------------------------------------------------------------
/// extra text to display when the info button is pressed.
/// -------------------------------------------------------------
	char info[100];
/// -------------------------------------------------------------
/// define whether this checklist will happen automatically in
/// quickstart mode
/// -------------------------------------------------------------
	bool automatic;
/// -------------------------------------------------------------
/// reference to the panel switch that must be thrown, used to
/// spawn reference box
/// -------------------------------------------------------------
	char item[100];
/// -------------------------------------------------------------
/// position the switch must be moved to.  Used for auto detect
/// of checklist complete.
/// -------------------------------------------------------------
	int position;
/// -------------------------------------------------------------
/// This is set to represent whether the step should be shown as
/// already complete.
/// -------------------------------------------------------------
	bool complete;
/// -------------------------------------------------------------
/// This is set to represent whether the step should be shown as
/// failed.
/// -------------------------------------------------------------
	bool failed;
/// -------------------------------------------------------------
/// This is an internal operator for the class.  The output should
/// not be expected to be representative of true equality between
/// the items, but rather is used for an internal check only.
/// -------------------------------------------------------------
	bool operator==(ChecklistItem);
};
/// -------------------------------------------------------------
/// Structure containing an active checklist "program"  This
/// structure contains the general group definition as well as a
/// complete list of the group's items.  It also contains an 
/// iterator that sits on the "current" checklist item for this
/// "program"
/// -------------------------------------------------------------
struct ChecklistContainer
{
/// -------------------------------------------------------------
/// Default Constructor.  Initializes an empty container.
/// -------------------------------------------------------------
	ChecklistContainer();
/// -------------------------------------------------------------
/// Constructor, requires a group to initialize from.
/// -------------------------------------------------------------
	ChecklistContainer(const ChecklistGroup &, BasicExcel &, const vector<ChecklistGroup> &);
/// -------------------------------------------------------------
/// Copy Constructor.
/// -------------------------------------------------------------
	ChecklistContainer(const ChecklistContainer &);
/// -------------------------------------------------------------
/// Assignment operator.
/// -------------------------------------------------------------
	void operator=(const ChecklistContainer &);
/// -------------------------------------------------------------
/// Checklist Group that this "program" is based on.
/// -------------------------------------------------------------
	ChecklistGroup program;
/// -------------------------------------------------------------
/// The list of elements in this "program"
/// -------------------------------------------------------------
	vector<ChecklistItem> set;
/// -------------------------------------------------------------
/// An iterator that rests on the current step of the program.
/// -------------------------------------------------------------
	vector<ChecklistItem>::iterator sequence;
private:
/// -------------------------------------------------------------
/// Initializer for the initializer.
/// -------------------------------------------------------------
	void initSet(const ChecklistGroup &,vector<ChecklistItem> &, BasicExcel &, const vector<ChecklistGroup> &);
};

#include "MFDconnector.h" //Has to be done here because fails further up.

/// -------------------------------------------------------------
/// This is the actual controller.  It exists once in each vessel
/// that implements the need to operate a checklist.  It keeps a 
/// set of available checklist groups that can be loaded to be
/// an active checklist "program"
/// -------------------------------------------------------------
class ChecklistController
{
public:
	ChecklistController();
	~ChecklistController();
/// -------------------------------------------------------------
/// Pass in a ChecklistItem with group and index initialized.
/// If group is initialized to a valid group, then it is assumed
/// that you want to start that group, and the returned item will
/// be the first item in the group.
/// If group is initialized to -1 then index becomes the index from
/// the "current" checklist item to return.
/// In the event that an essential checklist is running, a selected
/// checklist will not actually start running but get placed at the
/// top of the stack to be run when the essential checklist is done.
	bool getChecklistItem(ChecklistItem*);
/// -------------------------------------------------------------
/// Basic accessor for the list of available checklist items.
/// Access only when needed.  Not responsible for maintaining the
/// data.
/// -------------------------------------------------------------
	vector<ChecklistGroup> *getChecklistList();
/// -------------------------------------------------------------
/// This checklist item is failed.  In order to properly handle
/// this situation, we need to actually tell the controller instead
/// of just jumping away.  The controller will tell us where to go.
/// -------------------------------------------------------------
	bool failChecklistItem(ChecklistItem*);
/// -------------------------------------------------------------
/// This checklist item is complete.  In the event it is the item
/// currently in the iterator, it will simply move the iterator.
/// In the event it is a checklist down the road, it will simply 
/// display completed.
/// -------------------------------------------------------------
	bool completeChecklistItem(ChecklistItem*);
/// -------------------------------------------------------------
/// This allows setting of the autoComplete function which 
/// automatically detects if a step is already complete and marks
/// it as such, hiding it if it's already at the top of the list.
/// returns old state.
/// -------------------------------------------------------------
	bool autoComplete(bool);
	bool autoComplete();
	void clbkTimestep(double missionTime);
/// -------------------------------------------------------------
/// This method does default (empty) initialization.  Can be 
/// called to indicate that the Checklist controller should do 
/// nothing.  WARNING:  once this is called, you cannot call any
/// other init function!  Should NOT be called except in the case
/// you want to override the functions loaded by the scenario.
/// IE you want to use an unmanned flight.
/// -------------------------------------------------------------
	void init();
/// -------------------------------------------------------------
/// This method initializes any vessel for any mission.  Uses a
/// custom checklist file where the checkFile parameter is the
/// proper path to the file.  Otherwise, pass in null to indicate
/// should use default file.
/// WARNING:  once this is called, you cannot call any other init
/// function!
/// -------------------------------------------------------------
	bool init(char *checkFile);
/// -------------------------------------------------------------
/// called to save checklistController state.
/// -------------------------------------------------------------
	void save(FILEHANDLE scn);
/// -------------------------------------------------------------
/// called to load checklistController state.
/// -------------------------------------------------------------
	void load(FILEHANDLE scn);
/// -------------------------------------------------------------
/// Set the auto execute setting.  Note that this should ONLY be
/// used by the LEM and only when it is first created.
/// -------------------------------------------------------------
	bool autoExecute(bool set);
/// -------------------------------------------------------------
/// Get the auto execute setting.
/// -------------------------------------------------------------
	bool autoExecute();
/// -------------------------------------------------------------
/// Used to link checklist controller to a vessel, required to
/// Allow automated checklists as well as automatic checklist
/// selection and automatic completion detection.
/// -------------------------------------------------------------
	bool linktoVessel(VESSEL *vessel);
protected:
private:
	/// Auto complete flag.  If true, automatically complete the checklist.
	bool complete;
	///flag to prevent re-init of module.  Only the constructor can call 
	///init without locking the class in the given state.
	bool initCalled;
	///This is a "stackable queue" implemented by using either the push
	///front/push back functions to allow checklist groups to be lined
	///up for execution.
	deque<ChecklistContainer> action;
	///The active checklist group.
	ChecklistContainer active;
	///The list of all available checklist groups.
	vector<ChecklistGroup> groups;
	///The mission time at which the controller was last called.
	double lastMissionTime;
	///This is where actual "default" init happens.  Only the constructor calls this with false.
	bool init(bool final);
	///This determines whether or not the checklist gets auto executed.
	bool autoexecute;
	///Used to spawn new "program"
	bool spawnCheck(int group, bool fail, bool automagic = false);
	///Connector to the panel
	MFDConnector conn;
	/// Used to move forward through the elements.
	void iterate();
	///The file reference.
	char FileName[100];
	/// The actual file.
	BasicExcel file;
	///A temporary, frequently regenerated list of all availabe manually selectable checklists.
	vector<ChecklistGroup> groups_manual;
};

//Reenable 4018 warning
#pragma warning ( pop )
#endif