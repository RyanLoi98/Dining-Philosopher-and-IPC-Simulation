# 🍝 Dining Philosopher and IPC Simulation

**CPSC 457 Principles of Operating Systems | Fall 2023 | University of Calgary**

### 👨‍💻 Author

Ryan Loi

---

## 📌 Overview

This assignment explores **concurrency**, **synchronization**, and **inter-process communication (IPC)** through the implementation of the **Dining Philosophers problem** in a simulated environment. The project focuses on using **semaphores**, **shared memory**, and **message queues** to demonstrate process coordination, prevent deadlock, and handle race conditions in a concurrent system.

The assignment is split into two main sections:

---

## 🧩 Section 1: IPC and Process Creation

This section covers various **inter-process communication (IPC)** mechanisms, and is broken down into 6 parts stored in the section1 directory (so cd into this folder):

## Part 1: Process Creation with fork():

This part teaches how to create child processes.

  - Use `fork()` to create child processes.
  - Differentiate between parent and child processes based on `fork()` return value.
  - Each process prints its status.

#### 🔧 Compilation

```bash
gcc -o p1 p1.c -lrt
```

#### ▶️ Running the Program

```bash
./p1
```

### 📸 Screenshot
![Image description](https://i.imgur.com/CXGlQag.png)


---

## Part 2: IPC Using Message Queues:

This part teaches how to perform communication between processes using message queues.

  - Parent process sends messages to child via message queues.
  - Child process converts the received message to uppercase and sends it back to the parent process.
  - Both processes display messages for verification.


#### 🔧 Compilation

```bash
gcc -o p2 p2.c -lrt
```

#### ▶️ Running the Program

```bash
./p2
```

### 📸 Screenshot
![Image description](https://imgur.com/q24UxNf.png)


---


## Part 3: IPC Using Shared Memory:

This part teaches how to perform communication between processes using shared memory, we will be adapting the code from part 2 to achieve this goal.

  - Shared memory will be structured to house messages and any necessary flags or indicators to facilitate synchronized access.
  - Implement race condition prevention using semaphores.
  - Processes display messages exchanged via shared memory.



#### 🔧 Compilation

```bash
gcc -o p3 p3.c -lrt
```

#### ▶️ Running the Program

```bash
./p3
```

### 📸 Screenshot
![Image description](https://imgur.com/I0j0BWh.png)


---

## Part 4: IPC using Shared Memory with Process Coordination and Synchronization:

This part focuses on process coordination between multiple processes accessing shared resources.

  - Parent processes interacts with multiple child processes (minimum of 3) via shared memory.
  - Shared memory incorporate message buffer for child processes to deposit a message, and has an array to monitor child's status.
  - Parent periodically checks for the child's messages and displays them, then updates the status of the child to indicate that the message has been processed.
  - Synchronization implemented (via semaphores) to ensure:
        • Exclusive write access to shared memory for one child process at a time.
        • The parent process waits until a child completes its message before reading.
        • A child waits until its message is processed by the parent before writing a new one.


#### 🔧 Compilation

```bash
gcc -o p4 p4.c -lrt
```

#### ▶️ Running the Program

```bash
./p4
```

### 📸 Screenshot
![Image description](https://imgur.com/U6fzyXa.png)



---

## 🧩 Section 2: Dining Philosophers Problem

This section focuses on solving the classic **Dining Philosophers problem** (with a twist where these philosophers are in space so they are also called astronomers!) to simulate a scenario with concurrent processes (astronomers) sharing resources (quantum chopsticks). Now if you are not familiar with what the dining philosophers problem is - here is a recap: 

The Dining Philosophers Problem is a classic synchronization problem that demonstrates issues related to concurrency, resource sharing, and deadlock prevention. It involves  philosophers sitting around a table, each philosopher has a plate of food and two chopsticks (one on either side of the bowl, this means there are not enough chop sticks for each philosopher to have 2 in order to properly eat their food). To eat, each philosopher needs both chopsticks, but they can only pick up one chopstick at a time.

![Image description](https://imgur.com/zKWkyn4.png)


The challenge is to design a system where:

    - No philosopher starves (i.e., every philosopher gets a chance to eat).

    - There is no deadlock (i.e., no situation where all philosophers are holding one chopstick and waiting for the other).

    - There is no resource contention (i.e., multiple philosophers shouldn't attempt to use the same chopstick simultaneously).

This section is stored in the section2 directory so please cd into this folder.

### ✅ Section Requirements:

- **Symmetric Astronomers**:

  - Pick up both quantum chopsticks simultaneously.
  - Ensure a maximum 0.5-second time difference between acquiring chopsticks.

- **Asymmetric Astronomers**:

  - Pick up the right chopstick before the left one (maximum 1 second difference between picking up the right chopstick to picking up the left chopstick).
  - If unable to acquire both chopsticks in 2 seconds, release the right one to avoid starvation.

- **Deadlock Prevention**:

  - Implement strategies to avoid deadlocks where all astronomers wait indefinitely for chopsticks.
  - Use semaphores for synchronization and mutual exclusion.

- **Starvation Prevention**:

  - Ensure that no astronomer is left starving by managing access to chopsticks fairly.

- **Visualization**:

  - Simulate and visualize the state of each astronomer and chopstick (eating or contemplating) in the terminal.

- **Random Placement and Universal**:

  - Philosophers (symmetric and asymmetric) are randomly placed around the table.
  - The program is also universal so it can handle n number of philosophers.

#### 🔧 Compilation

```bash
gcc -o p1 p1.c -lrt
```

#### ▶️ Running the Program

```bash
./p1
```

This program runs indefinitely so use this command to stop it:

```bash
ctrl + c
```


### 📸 Screenshot
![Image description](https://imgur.com/KMqtp5H.png)

---

## Bonus

  - The bonus for this assignment features "greedy astronomers": Greedy astronomers must eat two times, drawn from a random quantum variable with average of 2*avg eat time, in a row before releasing both the quantum chopsticks at the same time.

  - The goal is to maximize overall throughput and ensure that no astronomer starves while accommodating the greedy astronomers’ preference for consecutive meals.


#### 🔧 Compilation

```bash
gcc -o bonus bonus.c -lrt
```

#### ▶️ Running the Program

```bash
./bonus
```

This program runs indefinitely so use this command to stop it:

```bash
ctrl + c
```

### 📸 Screenshot
![Image description](https://imgur.com/SJhLwwb.png)

---

## 🛠 Implementation Details

- **Language**: C
- **IPC**: Shared memory, message queues
- **Synchronization**: Semaphores for mutual exclusion
- **Concurrency**: `fork()` for creating processes, semaphores for coordinating access to shared resources
- **Random Delays**: Simulate randomness in the philosopher’s eating times to reflect realistic behavior
- **Error Handling**: Proper error handling for system calls and IPC resources

---

## 🔧 Compilation

To compile the program:

```bash
# Compile the IPC demonstration program
gcc -o ipc_demo ipc_demo.c -lrt

# Compile the Dining Philosophers simulation program
gcc -o dining_philosophers dining_philosophers.c -lrt
```

---

## ▶️ Running the Programs

### Part 1 – IPC and Process Communication

```bash
./ipc_demo
```

This program demonstrates the use of `fork()`, message queues, and shared memory for inter-process communication. The output displays messages exchanged between the parent and child processes, ensuring synchronization and message integrity.

### Part 2 – Dining Philosophers Simulation

```bash
./dining_philosophers
```

This program simulates the dining philosophers, ensuring deadlock prevention and fairness in resource allocation. The output shows each philosopher’s state (contemplating or eating), as well as the status of the chopsticks (available or taken).

---

## 🧠 Concepts Practiced

- **Process Creation and Management**: Using `fork()` to create child processes and understanding parent-child relationships.
- **Inter-Process Communication (IPC)**: Implementing message queues and shared memory for communication between processes.
- **Concurrency and Synchronization**: Solving race conditions, deadlock prevention, and ensuring proper process synchronization with semaphores.
- **Real-Time Simulations**: Mimicking real-world resource sharing scenarios, ensuring fairness and avoiding starvation.
- **Debugging Concurrent Systems**: Handling issues like deadlocks, race conditions, and ensuring proper synchronization.
