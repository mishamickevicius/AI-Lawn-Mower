/* 
Full Orchestrator FSM 
Connects all the components in the system
Should be implemented using a Task in the main component but not in app_main().
Will communicate using a queue system wide that can be added to by other FSMs and components.
The orchestrator will be responsible for handling queue events
*/