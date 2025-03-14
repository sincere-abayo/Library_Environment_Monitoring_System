<?php
include 'config.php';

$mode = $_POST['automation_mode'];
$query = "UPDATE system_settings SET automation_mode = $mode";
mysqli_query($conn, $query);

echo json_encode(['success' => true]);
