// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Cosimulation node responsible for simulating a tire system.
//
// =============================================================================

#include <algorithm>
#include <cstdio>

#include "chrono_vehicle/wheeled_vehicle/cosim/ChCosimManager.h"
#include "chrono_vehicle/wheeled_vehicle/cosim/ChCosimTireNode.h"
#include "chrono_vehicle/terrain/FlatTerrain.h"

namespace chrono {
namespace vehicle {

ChCosimTireNode::ChCosimTireNode(int rank, ChSystem* system, ChDeformableTire* tire, WheelID id)
    : ChCosimNode(rank, system), m_tire(tire), m_id(id) {}

void ChCosimTireNode::Initialize() {
    // Ghost wheel body (driven kinematically through messages from vehicle node)
    m_wheel = std::shared_ptr<ChBody>(m_system->NewBody());

    // Receive mass and inertia for the wheel body from the vehicle node
    {
        double props[4];
        MPI_Status status;
        MPI_Recv(props, 4, MPI_DOUBLE, VEHICLE_NODE_RANK, m_id.id(), MPI_COMM_WORLD, &status);
        if (m_verbose) {
            printf("Tire node %d. Recv from %d props = %g %g %g %g\n", m_rank, VEHICLE_NODE_RANK, props[0], props[1],
                   props[2], props[3]);
        }
        m_wheel->SetMass(props[0]);
        m_wheel->SetInertiaXX(ChVector<>(props[1], props[2], props[3]));
    }

    // Dummy terrain (needed for tire synchronization)
    m_terrain = std::make_shared<FlatTerrain>(0);

    // Ensure that tire contact is enabled and enforce TRIANGLE_MESH contact surface type
    //// TODO:
    ////   Since we are not interested in having any contact shapes on a tire node,
    ////   consider explicitly creating a contact surface here but never actually adding 
    ////   it to the tire underlying mesh.
    m_tire->EnableContact(true);
    m_tire->SetContactSurfaceType(ChDeformableTire::TRIANGLE_MESH);

    // Initialize the underlying tire
    m_tire->Initialize(m_wheel, m_id.side());

    // Create a mesh load for contact forces and add it to the tire's load container.
    auto contact_surface = std::static_pointer_cast<fea::ChContactSurfaceMesh>(m_tire->GetContactSurface());
    m_contact_load = std::make_shared<fea::ChLoadContactSurfaceMesh>(contact_surface);
    m_tire->GetLoadContainer()->Add(m_contact_load);

    // Send contact specification to terrain node
    {
        unsigned int props[2];
        props[0] = contact_surface->GetNumVertices();
        props[1] = contact_surface->GetNumTriangles();
        MPI_Send(props, 2, MPI_UNSIGNED, TERRAIN_NODE_RANK, m_id.id(), MPI_COMM_WORLD);
        if (m_verbose) {
            printf("Tire node %d. Send to %d props = %d %d\n", m_rank, TERRAIN_NODE_RANK, props[0], props[1]);
        }
    }
}

void ChCosimTireNode::Synchronize(double time) {
    // Send tire force to the vehicle node
    TireForce tire_force = m_tire->GetTireForce(true);
    double bufTF[9];
    bufTF[0] = tire_force.force.x;
    bufTF[1] = tire_force.force.y;
    bufTF[2] = tire_force.force.z;
    bufTF[3] = tire_force.moment.x;
    bufTF[4] = tire_force.moment.y;
    bufTF[5] = tire_force.moment.z;
    bufTF[6] = tire_force.point.x;
    bufTF[7] = tire_force.point.y;
    bufTF[8] = tire_force.point.z;
    MPI_Send(bufTF, 9, MPI_DOUBLE, VEHICLE_NODE_RANK, m_id.id(), MPI_COMM_WORLD);

    // Receive wheel state from the vehicle node
    double bufWS[14];
    MPI_Status statusWS;
    MPI_Recv(bufWS, 14, MPI_DOUBLE, VEHICLE_NODE_RANK, m_id.id(), MPI_COMM_WORLD, &statusWS);
    m_wheel_state.pos = ChVector<>(bufWS[0], bufWS[1], bufWS[2]);
    m_wheel_state.rot = ChQuaternion<>(bufWS[3], bufWS[4], bufWS[5], bufWS[6]);
    m_wheel_state.lin_vel = ChVector<>(bufWS[7], bufWS[8], bufWS[9]);
    m_wheel_state.ang_vel = ChVector<>(bufWS[10], bufWS[11], bufWS[12]);
    m_wheel_state.omega = bufWS[13];

    // Send tire location(s) to the terrain node
    //// TODO

    // Receive terrain force(s) from the terrain node
    //// TODO

    // Synchronize the ghost wheel and the tire
    m_wheel->SetPos(m_wheel_state.pos);
    m_wheel->SetRot(m_wheel_state.rot);
    m_wheel->SetPos_dt(m_wheel_state.lin_vel);
    m_wheel->SetWvel_par(m_wheel_state.ang_vel);
    m_tire->Synchronize(time, m_wheel_state, *m_terrain);
}

void ChCosimTireNode::Advance(double step) {
    double t = 0;
    while (t < step) {
        double h = std::min<>(m_stepsize, step - t);
        m_system->DoStepDynamics(h);
        t += h;
    }
    m_tire->Advance(step);
}

}  // end namespace vehicle
}  // end namespace chrono
