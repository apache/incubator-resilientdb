/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#![cfg_attr(
  all(not(debug_assertions), target_os = "windows"),
  windows_subsystem = "windows"
)]

use std::process::Command;

#[tauri::command]
fn check_docker() -> Result<String, String> {
  let output = Command::new("docker")
    .arg("--version")
    .output()
    .map_err(|e| e.to_string())?;
  if output.status.success() {
    Ok(String::from_utf8_lossy(&output.stdout).into_owned())
  } else {
    Err("Docker command failed".into())
  }
}

#[tauri::command]
fn build_docker_image() -> Result<String, String> {
  // Change working directory to the ansible project folder.
  let status = Command::new("docker")
    .current_dir("../resilientdb-ansible")
    .args(&["build", "-t", "resilientdb-ansible:latest", "."])
    .status()
    .map_err(|e| e.to_string())?;
  if status.success() {
    Ok("Docker image built successfully.".into())
  } else {
    Err("Docker image build failed.".into())
  }
}

#[tauri::command]
fn run_docker_container() -> Result<String, String> {
  // Check if the Docker image exists by listing its tags.
  let output = Command::new("docker")
    .args(&["images", "resilientdb-ansible", "--format", "{{.Tag}}"])
    .output()
    .map_err(|e| e.to_string())?;
  let tags = String::from_utf8_lossy(&output.stdout).trim().to_string();
  let image_exists = tags.lines().any(|line| line.trim() == "latest");

  if !image_exists {
    // Build the image if it doesn't exist.
    let build_status = Command::new("docker")
      .current_dir("../resilientdb-ansible")
      .args(&["build", "-t", "resilientdb-ansible:latest", "."])
      .status()
      .map_err(|e| e.to_string())?;
    if !build_status.success() {
      return Err("Docker image build failed.".into());
    }
  }

  // Remove any existing container.
  let _ = Command::new("docker")
    .args(&["rm", "-f", "resilientdb-container"])
    .output();

  // Run the container.
  let status = Command::new("docker")
    .current_dir("..")
    .args(&[
      "run",
      "--name", "resilientdb-container",
      "--privileged",
      "-v", "/sys/fs/cgroup:/sys/fs/cgroup:ro",
      "-p", "80:80",
      "-p", "18000:18000",
      "-p", "8000:8000",
      "-d",
      "resilientdb-ansible:latest",
    ])
    .status()
    .map_err(|e| e.to_string())?;
  if status.success() {
    Ok("Docker container started successfully.".into())
  } else {
    Err("Failed to start Docker container.".into())
  }
}

#[tauri::command]
fn stop_docker_container() -> Result<String, String> {
  let status = Command::new("docker")
    .args(&["stop", "resilientdb-container"])
    .status()
    .map_err(|e| e.to_string())?;
  if status.success() {
    Ok("Docker container stopped successfully.".into())
  } else {
    Err("Failed to stop Docker container.".into())
  }
}

/// Starts all ResilientDB services: 4 server nodes and 1 client.
#[tauri::command]
fn start_resilientdb() -> Result<String, String> {
  let services = [
    "resilientdb-kv@1",
    "resilientdb-kv@2",
    "resilientdb-kv@3",
    "resilientdb-kv@4",
    "resilientdb-client",
  ];
  for service in &services {
    let output = Command::new("docker")
      .args(&["exec", "resilientdb-container", "systemctl", "restart", service])
      .output()
      .map_err(|e| e.to_string())?;
    if !output.status.success() {
      return Err(format!(
        "Failed to start {}: {}",
        service,
        String::from_utf8_lossy(&output.stderr)
      ));
    }
  }
  Ok("ResilientDB services started successfully.".into())
}

/// Stops all ResilientDB services.
#[tauri::command]
fn stop_resilientdb() -> Result<String, String> {
  let services = [
    "resilientdb-kv@1",
    "resilientdb-kv@2",
    "resilientdb-kv@3",
    "resilientdb-kv@4",
    "resilientdb-client",
  ];
  for service in &services {
    let output = Command::new("docker")
      .args(&["exec", "resilientdb-container", "systemctl", "stop", service])
      .output()
      .map_err(|e| e.to_string())?;
    if !output.status.success() {
      return Err(format!(
        "Failed to stop {}: {}",
        service,
        String::from_utf8_lossy(&output.stderr)
      ));
    }
  }
  Ok("ResilientDB services stopped successfully.".into())
}

/// Start Crow HTTP service.
#[tauri::command]
fn start_crow() -> Result<String, String> {
  let output = Command::new("docker")
    .args(&["exec", "resilientdb-container", "systemctl", "restart", "crow-http"])
    .output()
    .map_err(|e| e.to_string())?;
  if output.status.success() {
    Ok("Crow service started successfully.".into())
  } else {
    Err(format!("Failed to start Crow: {}", String::from_utf8_lossy(&output.stderr)))
  }
}

/// Stop Crow HTTP service.
#[tauri::command]
fn stop_crow() -> Result<String, String> {
  let output = Command::new("docker")
    .args(&["exec", "resilientdb-container", "systemctl", "stop", "crow-http"])
    .output()
    .map_err(|e| e.to_string())?;
  if output.status.success() {
    Ok("Crow service stopped successfully.".into())
  } else {
    Err(format!("Failed to stop Crow: {}", String::from_utf8_lossy(&output.stderr)))
  }
}

/// Start GraphQL service.
#[tauri::command]
fn start_graphql() -> Result<String, String> {
  let output = Command::new("docker")
    .args(&["exec", "resilientdb-container", "systemctl", "restart", "graphql"])
    .output()
    .map_err(|e| e.to_string())?;
  if output.status.success() {
    Ok("GraphQL service started successfully.".into())
  } else {
    Err(format!("Failed to start GraphQL: {}", String::from_utf8_lossy(&output.stderr)))
  }
}

/// Stop GraphQL service.
#[tauri::command]
fn stop_graphql() -> Result<String, String> {
  let output = Command::new("docker")
    .args(&["exec", "resilientdb-container", "systemctl", "stop", "graphql"])
    .output()
    .map_err(|e| e.to_string())?;
  if output.status.success() {
    Ok("GraphQL service stopped successfully.".into())
  } else {
    Err(format!("Failed to stop GraphQL: {}", String::from_utf8_lossy(&output.stderr)))
  }
}

fn main() {
  tauri::Builder::default()
    .invoke_handler(tauri::generate_handler![
      check_docker,
      build_docker_image,
      run_docker_container,
      stop_docker_container,
      start_resilientdb,
      stop_resilientdb,
      start_crow,
      stop_crow,
      start_graphql,
      stop_graphql
    ])
    .run(tauri::generate_context!())
    .expect("error while running tauri application");
}