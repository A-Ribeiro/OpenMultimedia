package util;

import java.util.LinkedList;
import java.util.Queue;

public class ObjectPool<T> {

    protected Class<T> tClass;
    protected Queue<T> available;

    public ObjectPool(Class<T> aClass) {
        tClass = aClass;
        available = new LinkedList<>();
    }

    protected T newInstanceOfT() {
        try {
            return tClass.newInstance();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        }
        return null;
    }

    public T create(){
        synchronized (this) {
            if (available.size() == 0)
                return newInstanceOfT();
            else {
                T result = available.remove();
                return result;
            }
        }
    }

    public void release(T obj){
        synchronized (this) {
            available.add(obj);
        }
    }

}
