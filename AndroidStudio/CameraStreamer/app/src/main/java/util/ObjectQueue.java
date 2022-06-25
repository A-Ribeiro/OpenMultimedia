package util;

import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.Semaphore;

public class ObjectQueue<T> {

    protected Semaphore semaphore;
    protected Queue<T> queue;
    protected int dequeue_count;

    public ObjectQueue() {
        semaphore = new Semaphore(0);
        queue = new LinkedList<T>();
        dequeue_count = 0;
    }

    public void enqueue(T obj) {
        synchronized (this){
            queue.add(obj);
            semaphore.release();
        }
    }

    public T dequeueIgnoreSignal() {
        synchronized (this) {
            if (!queue.isEmpty())
                return queue.remove();
            return null;
        }
    }

    public T dequeue() {

        T result = null;

        synchronized (this){
            if (isSignaledInCurrentThread())
                return null;
            dequeue_count++;
        }

        try {
            semaphore.acquire();
            synchronized (this){
                if (isSignaledInCurrentThread() || queue.isEmpty())
                    result = null;
                else
                    result = queue.remove();
            }
        } catch (InterruptedException e) {
            result = null;
        }

        synchronized (this){
            dequeue_count--;
            return result;
        }
    }

    public int size() {
        return queue.size();
    }

    public boolean isSignaledInCurrentThread() {
        return Thread.currentThread().isInterrupted();
    }

    // to be used to process the remaning elements on the queue...
    //  it will signal the current queue...
    public Queue<T> getSignaledQueue(){
        synchronized (this) {
            Queue<T> result = queue;
            queue = new LinkedList<T>();
            semaphore.release(dequeue_count);
            semaphore = new Semaphore(0);
            dequeue_count = 0;
            return result;
        }
    }

}
