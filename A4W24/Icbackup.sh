#!/bin/bash

# Directory paths
backup_directory="/home/wakib/backup/*"
complete_backup_dir="/home/wakib/backup/cbw24"
incremental_backup_dir="/home/wakib/backup/ib24"
differential_backup_dir="/home/wakib/backup/db24"
log_file="/home/wakib/backup/backup.log"

# Initialize the backup number
complete_backup_number=1
incremental_backup_number=1
differential_backup_number=1

#storing backup files & their path
latest_complete_backup=""
latest_incremental_backup=""
latest_differential_backup=""

# Function to create backup and update log-Step 1
create_complete_backup() {
    timestamp=$(date +"%a %d %b %Y %I:%M:%S %p %Z")
    log_file_input="cbw24-$complete_backup_number.tar"
    complete_backup_file="$complete_backup_dir/$log_file_input"
    
    # Create the backup directory if it doesn't exist
    mkdir -p "$complete_backup_dir"
    
    # Check if the backup directory was successfully created
    if [ -d "$complete_backup_dir" ]; then
        tar -cf "$backup_dir/$complete_backup_file" -C /home/wakib/*
        echo "$timestamp $log_file_input was created" >> "$log_file"
        latest_complete_backup=$complete_backup_file
        echo "Complete Backup done!"
	    ((complete_backup_number++)) #Incrementing the complete abckup number for the next complete backup
    else
        echo "Error!"
    fi
}

# Function to create incremental backup-Step 2, 3 & 5
create_incremental_backup() {
    timestamp=$(date +"%a %d %b %Y %I:%M:%S %p %Z")
    log_file_input="ibw24-$incremental_backup_number.tar"
    incremental_backup_file="$incremental_backup_dir/$log_file_input"
    
    # Find files modified/created after the last backup
    if [ "$latest_incremental_backup" == "" ];then #incremental backup for the first time
        modified_files=$(find /home/wakib/* ! -path "$backup_directory" -type f -newer "$backup_dir/$latest_complete_backup")
    elif [ "$latest_differential_backup" != "" ];then #incremental backup after the differential backup.
        modified_files=$(find /home/wakib/* ! -path "$backup_directory" -type f -newer "$backup_dir/$latest_differential_backup")
        latest_incremental_backup=""
        latest_differential_backup=""
    else #incremental backup for the 2nd time.
        modified_files=$(find /home/wakib/* ! -path "$backup_directory" -type f -newer "$backup_dir/$latest_incremental_backup")
    fi
    
    # Check if any modified/created files exist
    if [ -n "$modified_files" ]; then
        # Create incremental backup if modified/created files exist
        mkdir -p "$incremental_backup_dir"
        tar -cf "$incremental_backup_file" $modified_files
        echo "$timestamp $log_file_input was created" >> "$log_file"
        latest_incremental_backup=$incremental_backup_file
        echo "Incremental backup done!"
	    ((incremental_backup_number++))
    else
        # Log a message if no modified/created files exist
        echo "$timestamp No changes-Incremental backup was not created" >> "$log_file"
        echo "No Incremental Backup has been done!"
    fi
}

# Function to create differential backup - Step 4
create_differential_backup() {
    timestamp=$(date +"%a %d %b %Y %I:%M:%S %p %Z")
    log_file_input="dbw24-$differential_backup_number.tar"
    differential_backup_file="$differential_backup_dir/$log_file_input"

    modified_files=$(find /home/wakib/* ! -path "$backup_directory" -type f -newer "$backup_dir/$latest_complete_backup")
    
    # Check if any modified/created files exist
    if [ -n "$modified_files" ]; then
        # Create differential backup if modified/created files exist
        mkdir -p "$differential_backup_dir"
        tar -cf "$differential_backup_file" $modified_files
        echo "$timestamp $log_file_input was created" >> "$log_file"
        latest_differential_backup=$differential_backup_file
        echo "Differential backup done!"
	    ((differential_backup_number++))
    else
        # Log a message if no modified/created files exist
        echo "$timestamp No changes-Differential backup was not created" >> "$log_file"
        echo "No Differential Backup has been done!"
    fi
}

# Main loop to run continuously
while true; do
    create_complete_backup
    # sleep 120
    sleep 30
    create_incremental_backup
    # sleep 120
    sleep 30
    create_incremental_backup
    # sleep 120
    sleep 30
    create_differential_backup
    # sleep 120
    sleep 30
    create_incremental_backup
done