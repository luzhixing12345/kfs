
# io

在讨论计算机时,我们实际上在使用的是**输入输出设备**.对于初次接触计算机的人来说,他们首先会注意到的是显示器上的图像和图形,并通过鼠标和键盘向计算机发出指令.对于终端用户而言,这些输入输出设备构成了他们与计算机交互的直接接口.

![20240329102322](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329102322.png)

然而,当我们深入学习计算机科学,例如计算组成原理或计算机系统基础时,我们会了解到CPU的核心作用, 它就是一个**不断执行指令的机器**, 取指令、译码和执行.这种认识揭示了一个本质的差异:我们日常使用的设备并不是计算机内部用于计算的核心部件.

计算机要为我们提供服务,就必须配备相应的输入输出设备,这样才能让人们真正地使用它.例如,我们需要打印机来将数据输出到纸张上,需要键盘来输入指令,以及需要显示器来查看信息.

对于任何一个CPU来说,要实现与外部设备的交互就需要精心设计的输入输出设备.这些设备本质上是类似的,它们使得CPU能够与外部世界进行交互.我们可以将输入输出设备视为计算机的"眼睛和耳朵",它们让计算机能够感知外部状态,并对外实施动作.

例如,显示器上每个像素的亮暗变化,都是对外部物理世界的一种影响.同样,键盘上的每一次按键和鼠标的每一次移动,都让计算机能够感知到物理世界中的变化.因此,**CPU与外设之间的交互最核心的功能就是数据的交换**.这种数据交换是实现计算机功能和用户交互的基础,是计算机系统不可或缺的一部分.

## 输入输出设备的原理

在讨论计算机系统中的CPU与输入输出(I/O)设备的关系时,我们可以从CPU的角度来理解这些设备的工作原理.对于CPU而言,所有的I/O设备在本质上可以被视为一根线,这根线上承载着数据,从设备端连接到CPU的引脚, 并且**以一种与CPU预先约定好的方式进行数据交换**.这种交换方式定义了输入(Input)和输出(Output)的概念:输入是将数据送入CPU进行处理,而输出则是将CPU处理后的数据发送到外部设备或控制器.

> 下图为 intel CPU 的引脚对照图

![20240329102957](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329102957.png)

因此从 CPU 的视角来看, **IO设备就是一个能和CPU交换数据的接口/控制器**, 通过几组约定好功能的线和 CPU 相连, 通过握手信号从线上读出/写入数据. 每一组线有自己的地址, CPU可以直接使用指令(in/out/MMIO)和设备交换数据, 设备会被抽象为一组 (状态, 命令, 数据) 的接口, CPU 完全不管设备是如何实现的

> 状态寄存器(Status)、命令寄存器(Command)和数据寄存器(Data).状态寄存器用于反映设备的状态,例如磁盘是否正在写数据、设备是否忙碌等.命令寄存器允许我们对设备执行操作,比如发送指令让设备执行特定任务.数据寄存器则用于在CPU和设备之间传输数据.

所以计算机系统中的设备并没有神秘之处,**它们的本质是一组寄存器的集合**,这些寄存器代表了**设备的状态、控制命令和数据传输**等功能.这些寄存器被映射到特定的地址空间中,无论是内存地址空间还是专门的I/O地址空间,这都是硬件制造商和操作系统之间的一种约定.

> CPU 可以向访问内存一样通过一个地址来访问外部设备的寄存器, 这些地址是由硬件厂商和操作系统通过协调统一的
>
> 下图为 PS/2 键盘控制器的接口, 它被硬编码到两个 I/O port: 0x60 (data), 0x64 (status/command), 可以通过这两个端口来读写数据
>
> ![20240329103933](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329103933.png)

然而,在实际的计算机系统中,我们并不会为每一个I/O设备单独拉出一根线连接到CPU,因为这样会导致系统过于复杂且不实用.例如,我们的系统可能包含键盘、鼠标、U盘、蓝牙控制器、打印机等多种设备.如果为每个设备都单独连接一根线,CPU的结构将变得极其复杂.

> 例如,在IBM PC兼容机的设计中,为了确保兼容性,某些地址必须映射到特定的I/O设备的寄存器,如键盘控制器和磁盘控制器.这种设计在一定程度上限制了系统的扩展性,因为它预设了系统能够识别和使用的I/O设备类型.

然而随着技术的发展和新设备的出现,如新型打印机、游戏控制器等,我们需要一种更加灵活和可扩展的方式来管理I/O设备.这就引出了**总线(bus)**的概念.**总线是一种特殊的I/O设备,它负责统一管理和协调所有连接到它的其他I/O设备**.CPU不再需要直接与每个设备通信,而是通过总线来访问和控制这些设备, **提供设备的注册**和**地址到设备的转发**

总线通过**分配唯一的地址给每个设备**,使得CPU可以**通过总线控制器来访问任何一个设备**.当多个设备需要与CPU通信时,总线控制器会根据中断优先级来协调,确保CPU能够及时响应最重要的事件.此外,总线控制器还能够识别发生中断的设备,进一步增强了系统的灵活性和响应能力.

![20240329110305](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329110305.png)

通过这种设计,计算机系统不仅能够支持现有的I/O设备,还**为未来的设备扩展留下了充足的空间**.这种基于总线的I/O管理方式,使得计算机系统能够适应不断变化的技术环境,支持各种新型设备,从而保持了系统的长期可用性和先进性.

> 今天 PCIe 总线肩负了这个任务, 总线还可以桥接其他总线(USB), 可以使用下面的命令查看系统总线上的设备
>
> ```bash
> lspci -tv
> ```
>
> 总线的基础概念很简单, 但是实际设计异常复杂, 需要考虑例如电气特性, burst 传输, 中断等等因素
> 
> 关于总线的更多讨论见 arch/bus, 这里不详细展开

## 中断

中断控制器是计算机系统中一项特殊而重要的设计,它为操作系统的稳定运行和高效管理提供了基础.在计算机系统中,应用程序不断执行指令,但并不会因此导致系统失去响应,这得益于中断机制的存在.

一个没有中断的计算机体系是**决定论**的: **得知某个时刻CPU和内存的全部数据状态,就可以推衍出未来的全部过程**.这样的计算机无法交互,只是个执行指令的工具.

添加中断后,计算机指定了会兼容哪些外部命令,并设定服务程序,这种服务可能打断当前任务.这使得CPU"正在执行的程序"与"随时可能发生的服务",二者形成了异步关系,外界输入的引入使得计算机程序**不再是决定论.由人实时控制的中断输入**,是无法预测的.再将中断响应规则化,推广开,非计算机科学人群就能控制计算机,发挥创造力.电竞鼠标微操,数码板绘,音频输入合成,影像后期数值调整,键盘点评天下大势,这些都不是定势流程,是需要人实时创造参与其中的事件,就由中断作为载体,与计算机结合了起来.**中断就是处理器的标准输入接口**.

因此**中断的本质是处理器对外开放的实时受控接口**,例如IRQ线,这是一个边缘触发且低电平有效的信号线.**当这条线被激活时,CPU必须响应这个特殊的事件**,就像核弹发射按钮被按下时,系统必须立即采取行动一样.在这个过程中,中断控制器扮演了一个关键角色.

![20240329102848](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329102848.png)

而负责处理中断的**中断控制器**内部包含一系列的控制寄存器, 连接到 CPU 的中断引脚. 中断控制器中保存 5 个寄存器 (cs, rip, rflags, ss, rsp), 当设备发送中断请求的时候由中断控制器向 CPU 发送中断信号, CPU 跳转到中断向量表对应项执行.

在我们的计算机系统中,许多外围设备,如键盘、网卡和鼠标等,都有能力产生中断.这些中断实际上是通过各自的信号线来实现的,通常这些线在接收到低电平信号时被激活.当这些设备中的任何一个需要CPU的注意时,它就会通过这根线发送中断请求.所有的这些中断请求线最终都会连接到一个中断控制器上.这个中断控制器是一个**可编程的设备**(PIC, programmable interrupt controller),它的主要任务是对这些中断请求进行**裁决**.例如,如果同时有两个或多个中断请求发生,中断控制器会根据每个中断的优先级来决定哪个请求应该首先被处理.这个优先级设置确保了更重要的任务能够优先得到处理.

> 在实时系统中,这种优先级处理尤为重要,因为系统需要确保关键任务能够及时得到响应.中断控制器还具备中断屏蔽的功能,允许CPU在处理某些紧急任务时暂时忽略那些优先级较低的中断,从而保持系统的稳定性和效率.

例如 [Intel 8259 PIC](https://en.wikipedia.org/wiki/Intel_8259) 中断控制器就可以编程控制哪些中断可以屏蔽, 多个中断请求的优先级等等

![20240329115227](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329115227.png)

现如今 8259 PIC 已经远远不能满足日益复杂的 CPU 了(比如说多处理器多核), 诞生了 **APIC**(Advanced PIC)

APIC是一种高级可编程中断控制器,它提供了比传统中断控制器更为强大和灵活的中断处理能力.APIC通常集成在现代芯片组或北桥中,它支持多处理器系统中的中断管理和协调.APIC的主要特点包括:

1. **多处理器支持**:APIC允许**多个CPU之间进行中断通信**,这对于确保系统整体性能和响应性至关重要.
2. **可编程性**:APIC提供了丰富的编程接口,允许操作系统根据需要配置中断优先级、中断向量和中断屏蔽等.
3. **中断向量分配**:APIC可以为每个中断源分配唯一的向量号,这有助于操作系统更有效地处理中断.
4. **性能优化**:APIC通过减少CPU之间的中断冲突和优化中断处理流程,提高了系统的整体性能.

> 例如对于多个 CPU 核心, 每一个核心运行一个线程. 此时如果使用 mmap 分配了一块内存, 此时这块内存地址空间在两个 CPU 上都是可用的. 当某一个 CPU 核心上的线程执行了 munmap 之后, 此时操作系统会让该 CPU 核心采用**核间中断**(IPI, Inter-Processor Interrupt)的方式向其他执行线程的发送一个 **tlb shootdown**(详细讨论见 mm/tlb), 之后他们的页表会被同步, 其他CPU上的内存也会被消除映射

LAPIC(local APIC)是APIC的一种,它是集成在每个CPU内部的中断控制器.LAPIC为每个CPU提供了专用的中断处理能力,使得每个处理器都能够独立地处理中断,而不需要主控制器的介入.LAPIC的主要特点包括:

1. **本地化**:每个CPU都有自己的LAPIC,这样可以减少处理器之间的通信开销,提高中断处理速度.
2. **专用资源**:LAPIC为每个CPU提供了专用的资源,如定时器、性能计数器和中断向量表,使得每个处理器都能够独立地配置和管理自己的中断.
3. **高级中断管理**:LAPIC支持高级中断管理功能,如中断优先级、中断屏蔽和中断转发等.
4. **多线程支持**:LAPIC能够支持多线程环境下的中断处理,确保线程间的同步和资源共享.

APIC和LAPIC在现代计算机系统中提供了高效、灵活的中断处理机制,它们使得多处理器系统能够更好地管理和响应各种中断请求,从而提高了系统的整体性能和稳定性.通过这些先进的中断控制器,操作系统能够实现更精细的中断管理,优化资源分配,提升用户体验.

> 关于中断的更多讨论见 arch/interrupt, 这里不详细展开

## DMA

在计算机系统中,当需要传输大量数据时,如写入1GB的数据到磁盘,CPU可能会花费大量时间来处理这个任务.这是因为,即使磁盘已经准备好接收数据,CPU仍需执行一个庞大的循环,逐字节地将数据从内存复制到磁盘上.

假设此时 CPU 想要从磁盘中读出 1GB 的数据, 整个流程如下所示

1. CPU 向南桥芯片组发送指令, 读取某一个地址的数据, 南桥将操作转发给磁盘控制器
2. 磁盘将数据读出, 通过 CPU 写入到内存的某段空间中
3. 循环执行, 全部完成后 CPU 再从内存中读出数据

![20240329160646](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329160646.png)

我们发现最大的问题在于从磁盘读出数据的需要经过 CPU 才能被写入内存, 这个过程不仅耗时,而且会占用CPU的大量资源,使其无法同时处理其他任务.

> 从内存写入数据的过程类似, 也需要经过 CPU 完成数据的写入指令

为了解决这个问题,我们可以设想一种专门的硬件设备,**专门负责处理内存之间的数据传输**,从而释放CPU的资源.这种设备被称为**DMA**(Direct Memory Access)控制器.DMA控制器可以被设计得相对简单,因为它**只负责执行内存复制操作**, 一共四种情况: 外设到内存;内存到外设;内存到内存;外设到外设. 它不需要像通用CPU那样具备复杂的指令集,而是可以通过硬编码的方式,将内存复制的逻辑直接嵌入到电路中.

DMA控制器的工作原理是使用一个计数器和两个地址指针.在每个时钟周期,它会从源地址读取数据,写入目标地址,然后更新计数器和地址指针.这样的固定逻辑使得DMA控制器非常高效,特别适合处理大量数据传输的任务.

> 大部分的设备都是 Slave 设备, 也就是只能接受来自其他处理器的信号做出反应. DMA 控制器是一个 Master 设备, 它可以主动发出指令执行操作

1. CPU 向 DMA 发送指令, 告知移动数据的 (src, dst, length), 交由 DMA 处理, **CPU 继续工作**
2. DMA 从磁盘中读出数据, 通过 DMA 控制器和内存总线写入内存中的一片区域
3. DMA 通过中断告知 CPU 已经完成数据传输
4. CPU 从内存中读出数据

![20240329172122](https://raw.githubusercontent.com/learner-lu/picbed/master/20240329172122.png)

现代计算机系统中的PCI总线就支持DMA操作.这意味着,像显卡、网卡等高速设备,都可以利用DMA控制器来执行数据传输,而不需要占用CPU资源.例如,显卡可以通过DMA控制器直接与系统内存通信,进行高速的数据读写操作.通过使用DMA控制器等专门的硬件设备,可以优化数据传输过程,提高系统的整体性能.这样,CPU就可以专注于执行更复杂的计算任务,而不必被大量数据传输所拖累.

## 参考

- [输入输出设备原理 (总线、DMA、GPU) [南京大学2023操作系统-P26] (蒋炎岩)](https://www.bilibili.com/video/BV12T41147oE/)
- [计算机上"中断"的本质是什么?的回答](https://www.zhihu.com/question/21440586/answer/991259675)