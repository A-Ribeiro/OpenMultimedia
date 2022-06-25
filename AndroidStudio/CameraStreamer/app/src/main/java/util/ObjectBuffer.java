package util;

public class ObjectBuffer {

    protected byte[] data;
    protected int size;
    protected int allocSize;
    protected int writePosition;
    protected int readPosition;

    public ObjectBuffer(){
        data = null;
        size = 0;
        allocSize = 0;
        writePosition = 0;
        readPosition = 0;
    }

    public void setSize(int _size) {
        synchronized (this) {
            if (_size > allocSize) {
                allocSize = _size;
                data = new byte[allocSize];
            }
            size = _size;
            writePosition = 0;
            readPosition = 0;
        }
    }

    public byte[] getArray(){
        return data;
    }

    public int getSize() {
        return size;
    }

    public void resetWrite() {
        synchronized (this) {
            writePosition = 0;
        }
    }

    public void resetRead() {
        synchronized (this) {
            readPosition = 0;
        }
    }

    public void arrayReadFrom(byte[] _data, int start, int length){
        synchronized (this) {
            if (start + length > _data.length)
                return;
            if (writePosition + length > data.length)
                return;
            System.arraycopy(_data, start, data, writePosition, length);
            writePosition += length;
        }
    }

    public void arrayWriteTo(byte[] _data, int start, int length){
        synchronized (this) {
            if (start + length > _data.length)
                return;
            if (readPosition + length > data.length)
                return;
            System.arraycopy(data, readPosition, _data, start, length);
            readPosition += length;
        }
    }

}
