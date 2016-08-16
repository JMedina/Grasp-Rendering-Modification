#include <openrave-core.h>
#include <vector>
#include <cstring>
#include <sstream>

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

using namespace OpenRAVE;
using namespace std;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define usleep(micro) Sleep(micro/1000)
#endif

void SetViewer(EnvironmentBasePtr penv, const string& viewername)
{
    ViewerBasePtr viewer = RaveCreateViewer(penv,viewername);
    penv->AddViewer(viewer);
    viewer->main(true);
}

int main(int argc, char ** argv)
{
    string scenefilename = "data/lab1.env.xml";
    string viewername = "qtcoin";
    RaveInitialize(true);
    EnvironmentBasePtr penv = RaveCreateEnvironment();
    penv->SetDebugLevel(Level_Debug);

    boost::thread thviewer(boost::bind(SetViewer,penv,viewername)); // create the viewer
    usleep(300000); // wait for the viewer to init

    penv->Load(scenefilename);
    vector<RobotBasePtr> vrobots;
    penv->GetRobots(vrobots);
    RobotBasePtr probot = vrobots.at(0);
    std::vector<dReal> q;

    while(1) {
        {
            EnvironmentMutex::scoped_lock lock(penv->GetMutex()); // lock environment

            TrajectoryBasePtr traj = RaveCreateTrajectory(penv,probot->GetDOF());
            probot->GetDOFValues(q); // get current values
            traj->AddPoint(TrajectoryBase::TPOINT(q,probot->GetTransform(),0.0f));
            q[RaveRandomInt()%probot->GetDOF()] += RaveRandomFloat()-0.5; // move a random axis

            // check for collisions
            {
                RobotBase::RobotStateSaver saver(probot); // add a state saver so robot is not moved permenantly
                probot->SetDOFValues(q);
                if( penv->CheckCollision(RobotBaseConstPtr(probot)) ) {
                    continue; // robot in collision at final point, so reject
                }
            }

            traj->AddPoint(TrajectoryBase::TPOINT(q,probot->GetTransform(),2.0f));
            traj->CalcTrajTiming(probot,TrajectoryBase::CUBIC,false,false); // initialize the trajectory structures
            probot->GetController()->SetPath(traj);
            // setting through the robot is also possible: probot->SetMotion(traj);
        }
        // unlock the environment and wait for the robot to finish
        while(!probot->GetController()->IsDone()) {
            usleep(1000);
        }
    }

    thviewer.join(); // wait for the viewer thread to exit
    penv->Destroy(); // destroy
    return 0;
}