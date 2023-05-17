/// <reference types="node" />
declare module 'gpt4all-ts';

//todo
declare class LLModel {

    constructor(path: string);
    /**
     * Get the size of the internal state of the model.
     * NOTE: This state data is specific to the type of model you have created.
     * @param model A pointer to the llmodel_model instance.
     * @return the size in bytes of the internal state of the model
     */
    stateSize(): number;

    threadCount() : number;

    setThreadCount(newNumber: number): void
}

export { LLModel }
