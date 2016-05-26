a) условные

mutex mtx;
condition_variable cv_left;
condition_variable cv_right;

int last_thr = 0;

void step_left() {
    unique_lock<mutex> _mtx(mtx);
    while (last_thr == 0)
            cv_right.wait(_mtx);
    last_thr = 0;
    //что-то делает
    cv_left.notify_one();
}

void step_right() {
    unique_lock<mutex> _mtx(mtx);
    while (last_thr == 1)
            cv_left.wait(_mtx);
    last_thr = 1;
    //что-то делает
    cv_right.notify_one();
}

void cv_enter() {
    thread thread_0(step_left);
    thread thread_1(step_right);
    thread_0.join();
    thread_1.join();
}

b) Семафоры

semaphore sem_left(1);
semaphore sem_right(0);

void step_left(int i) {
  sem_right.wait();
  //что-то делает
  sem_left.signal();
}
void walkingRight(int i) {
    sem_left.wait();
    //что-то делает
    sem_right.signal();
}
void SemaphoreWalking() {
    thread thread_0(step_left, 0);
    thread thread_1(step_right, 1);

    thread_0.join();
    thread_1.join();
}

