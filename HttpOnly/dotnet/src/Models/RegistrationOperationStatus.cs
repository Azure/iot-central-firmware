public struct RegistrationOperationStatus {
    public string operationId { get; set; }
    public DeviceRegistrationResult registrationState { get; set; } 
    public DeviceEnrollmentStatus? status { get; set; }
}

public class DeviceRegistrationResult
{
    public string assignedHub { get; set; }
    public string createdDateTimeUtc { get; set; }
    public string deviceId { get; set; }
    public int errorCode { get; set; }
    public string errorMessage { get; set; }
    public string etag { get; set; }
    public string lastUpdatedDateTimeUtc { get; set; }
    public object payload { get; set; }
    public string registrationId { get; set; }
    public DeviceEnrollmentStatus status { get; set; }
    public DeviceEnrollmentSubStatus subStatus { get; set; }

    //unimplemented: symmetricKey, tpm, x509
}

public enum DeviceEnrollmentSubStatus {
    deviceDataMigrated,
deviceDataReset,
initialAssignment
}

public enum DeviceEnrollmentStatus {
    assigned,
    assigning,
    disabled,
    failed,
    unassigned
}