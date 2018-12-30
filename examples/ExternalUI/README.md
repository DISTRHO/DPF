# External UI example

This example will show how to use an external / remote UI together with DPF.<br/>

The Plugin has a shell script that calls kdialog and uses qdbus for sending values to it.<br/>
It is a very ugly way to show a remote UI (using a shell script!), but it is only to prove the point.<br/>

Note that everything regarding external UIs is still a bit experimental in DPF.<br/>
There is Unix-specific code in there.<br/>

If this is something you are interested on using and contributing to, please let us know.<br/>
