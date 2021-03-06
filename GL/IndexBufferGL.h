#pragma once

#include "../geometry/IndexBuffer.h"
#include "../common/gl.h"

namespace Core {

    class IndexBufferGL final: public IndexBuffer {
    public:
        IndexBufferGL(UInt32 size);
        virtual ~IndexBufferGL();
        Int32 getBufferID() const;
        void setIndices(UInt32 * indices) override;
        void initIndices() override;
        UInt32 getSize();
    private:
        void destroy();
        GLuint bufferID;
    };

}
