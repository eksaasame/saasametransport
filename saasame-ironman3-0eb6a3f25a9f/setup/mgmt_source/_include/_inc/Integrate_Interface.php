<?php

interface SaasameInterface
{
    public function CreateInstance($ImageId, $InstanceType);
    
    public function DeleteInstance( $instance );

    public function ListInstances();

    public function GetInstanceDetail( );

    public function CreateVolume();

    public function DeleteVolume( $vol );

    public function ListVolume();

    public function CreateSnapshot( $DiskId );

    public function CreateVolFromSnapshot();

    public function DeleteSanpshot();

    public function ListSnapshot();

    public function AttachVolToInstance();

    public function DetachVolFromInstance();

    public function CheckConnect();

    public function GetSnapshotList( );

    public function CreateMachine();
}